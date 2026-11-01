#include "cli_parser.h"

#include "libsokketter.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <sstream>
#include <type_traits>
#include <vector>

namespace {
    /**
     * @brief rewrites underscores to dashes in long-option tokens, e.g. --device_at_index becomes
     * --device-at-index.
     *
     * @attention CLI11's ignore_underscore() strips underscores from both the registered name and
     * the input, so it cannot alias an underscored spelling to a dashed option name. This pass
     * provides that aliasing. It covers tokens that begin with "--" as well as Windows-style
     * tokens that begin with "/" (enabled via allow_windows_style_options()), so that spellings
     * such as /device_at_index are aliased too.
     * @attention as a consequence, option values that begin with "/" (paths such as /dev/ttyUSB_0)
     * are also rewritten; such values are not expected for any current option. Negative numbers
     * and bare serials are unaffected.
     * @attention stops at key-value splitter '='.
     */
    auto normalize_cli_argument(std::string argument) -> std::string
    {
        if (argument.rfind("--", 0) == 0 || (!argument.empty() && argument.front() == '/'))
        {
            const auto separator = argument.find('=');
            const auto end = (separator == std::string::npos)
                                 ? argument.end()
                                 : argument.begin() + static_cast<std::ptrdiff_t>(separator);
            std::replace(argument.begin(), end, '_', '-');
        }

        return argument;
    }

    /**
     * @brief reports whether a concrete device type belongs to the given included-types mask.
     *
     * @attention power_strip_type is a scoped bit-flag enum with no operator overloads, so the
     * membership test is spelled once here and reused by both the display filter and the
     * --include-device-types token parser to keep the two in lockstep.
     */
    auto type_matches(sokketter::power_strip_type device_type,
        sokketter::power_strip_type included_types) -> bool
    {
        using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;
        return (static_cast<underlying_type>(device_type) &
                   static_cast<underlying_type>(included_types)) != 0;
    }

    /**
     * @brief returns the union of two power_strip_type bit-flag masks.
     *
     * @attention companion to type_matches; keeps the bit-flag arithmetic in one place since the
     * scoped enum provides no operator|.
     */
    auto type_union(sokketter::power_strip_type left, sokketter::power_strip_type right)
        -> sokketter::power_strip_type
    {
        using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;
        return static_cast<sokketter::power_strip_type>(static_cast<underlying_type>(left) |
            static_cast<underlying_type>(right));
    }

    /**
     * @brief the device filter used by the list subcommand by default and by the power subcommand
     * when resolving --device-at-index.
     *
     * @attention both the printed numbering and the index resolution go through this filter, which
     * is what makes the contract "index N addresses the device printed as N by list" hold.
     * Ethernet devices are addressable only via --device-with-serial.
     */
    auto default_display_filter() -> sokketter::device_filter
    {
        sokketter::device_filter filter;
        filter.included_types = sokketter::power_strip_type::USB_DEVICES;
        return filter;
    }

    /**
     * @brief returns the exact device list the user sees for the given filter, in the exact order
     * it is printed.
     *
     * @attention the library's devices(filter) only uses the filter to select which communications
     * to enumerate and returns the whole persisted database, so the CLI applies the type filter
     * client-side to make display and index resolution agree.
     * @attention TEST_DEVICE is not part of any type group; the library returns fake devices
     * regardless of the filter, so they stay visible here as well.
     */
    auto visible_devices(const sokketter::device_filter &filter)
        -> std::vector<std::shared_ptr<sokketter::power_strip>>
    {
        std::vector<std::shared_ptr<sokketter::power_strip>> visible;
        for (const auto &device : sokketter::devices(filter))
        {
            if (device->configuration().type == sokketter::power_strip_type::TEST_DEVICE ||
                type_matches(device->configuration().type, filter.included_types))
            {
                visible.push_back(device);
            }
        }
        return visible;
    }

    /**
     * @brief trims leading and trailing whitespace from an option value token.
     *
     * @attention trimming (not stripping): interior whitespace is kept so that malformed values
     * such as "PASS WORD" are rejected with a diagnostic that echoes what was actually typed,
     * rather than silently repaired into a valid one.
     */
    auto trim_option_token(const std::string &token) -> std::string
    {
        const auto first = std::find_if_not(token.begin(), token.end(),
            [](unsigned char character) { return std::isspace(character) != 0; });
        const auto last = std::find_if_not(token.rbegin(), token.rend(),
            [](unsigned char character) { return std::isspace(character) != 0; }).base();

        /**
         * @attention an all-whitespace token leaves first at end() and last at begin(), i.e. a
         * reversed range, which std::string would reject; such a token trims to empty.
         */
        return first >= last ? std::string() : std::string(first, last);
    }

    /**
     * @brief parses the comma-separated --include-device-types value shared by the list and power
     * subcommands into a bit-flag mask.
     *
     * @attention tokens are split on comma, whitespace-trimmed and matched case-insensitively;
     * empty tokens (including a trailing comma) and unknown tokens are rejected with identical
     * error text on stderr, so both subcommands report the same diagnostics.
     * @attention returns false when the value is malformed; selected_types is then unspecified.
     */
    auto parse_device_types(const std::string &raw_value,
        sokketter::power_strip_type &selected_types) -> bool
    {
        selected_types = sokketter::power_strip_type::UNKNOWN;

        std::istringstream token_stream(raw_value);
        std::string token;
        while (std::getline(token_stream, token, ','))
        {
            token = trim_option_token(token);

            std::transform(
                token.begin(), token.end(), token.begin(), [](unsigned char character) {
                    return static_cast<char>(std::tolower(character));
                });

            if (token.empty())
            {
                std::cerr << "Empty device type in --include-device-types."
                          << device_type_error_suffix() << std::endl;
                return false;
            }

            if (token == "usb")
            {
                selected_types =
                    type_union(selected_types, sokketter::power_strip_type::USB_DEVICES);
            }
            else if (token == "ethernet" || token == "lan")
            {
                selected_types =
                    type_union(selected_types, sokketter::power_strip_type::ETHERNET_DEVICES);
            }
            else
            {
                std::cerr << "Unknown device type: " << token << "." << device_type_error_suffix()
                          << std::endl;
                return false;
            }
        }

        /**
         * @attention getline never yields a final empty token when the value ends with a
         * comma, so a trailing comma must be rejected explicitly.
         */
        if (!raw_value.empty() && raw_value.back() == ',')
        {
            std::cerr << "Empty device type in --include-device-types."
                      << device_type_error_suffix() << std::endl;
            return false;
        }

        if (selected_types == sokketter::power_strip_type::UNKNOWN)
        {
            std::cerr << "No device types were specified." << device_type_error_suffix()
                      << std::endl;
            return false;
        }

        return true;
    }

    /**
     * @brief parses the --authentication-type value of the login subcommand.
     *
     * @attention the value is whitespace-trimmed and matched case-insensitively; only PASSWORD
     * is currently supported, so anything else is rejected with a fixed diagnostic listing the
     * available types.
     * @attention empty or whitespace-containing values (after trimming) are rejected explicitly:
     * silently stripping interior whitespace would accept malformed values such as "PASS WORD".
     * @attention returns false when the value is malformed; selected_type is then unspecified.
     */
    auto parse_authentication_type(const std::string &raw_value,
        sokketter::power_strip_authentication_type &selected_type) -> bool
    {
        std::string token = trim_option_token(raw_value);

        const auto has_whitespace = std::any_of(token.begin(), token.end(),
            [](unsigned char character) { return std::isspace(character) != 0; });
        if (token.empty() || has_whitespace)
        {
            std::cerr << "The --authentication-type value may not be empty or contain whitespace."
                      << authentication_type_error_suffix() << std::endl;
            return false;
        }

        std::transform(token.begin(), token.end(), token.begin(), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });

        if (token == "password")
        {
            selected_type = sokketter::power_strip_authentication_type::PASSWORD_ONLY;
            return true;
        }

        /**
         * @attention the diagnostic echoes the caller's raw spelling, not the normalised token:
         * quoting something the user did not type invites confusion about what was actually read.
         */
        std::cerr << "Unknown authentication type: " << raw_value << "."
                  << authentication_type_error_suffix() << std::endl;
        return false;
    }

    /**
     * @brief the device-addressing option pair attached to every subcommand that selects a single
     * device (power, login, logout).
     */
    struct device_access_options
    {
        CLI::Option *index = nullptr;
        CLI::Option *serial = nullptr;
    };

    /**
     * @brief registers the --device-at-index / --device-with-serial option group on the given
     * subcommand, backed by the provided shared storage.
     *
     * @attention all subcommands with device access get identical groups so the addressing
     * contract (mutual exclusion, underscore aliases, group help handling) is defined once.
     */
    auto add_device_options(CLI::App &command, size_t &device_index, std::string &device_serial)
        -> device_access_options
    {
        auto device_group =
            command.add_option_group("--device-at-index or --device-with-serial");

        auto option_device_index = device_group->add_option("--device-at-index,-i", device_index);
        auto option_device_serial =
            device_group->add_option("--device-with-serial,-n", device_serial);

        option_device_index->ignore_underscore();
        option_device_serial->ignore_underscore();
        option_device_index->excludes(option_device_serial);
        option_device_serial->excludes(option_device_index);

        /**
         * @warning option groups are nameless subcommands and inherit a help flag from the parent.
         * That flag captures --help before it can fall through to the application level, which
         * breaks help precedence (e.g. "power --help" fails the required subcommand check instead
         * of showing help). Removing it lets --help fall through to the application-level help
         * flag.
         */
        device_group->set_help_flag();
        device_group->set_help_all_flag();

        return {option_device_index, option_device_serial};
    }

    /**
     * @brief checks that one of the device-addressing options was provided and reports the
     * standard diagnostic when neither was.
     *
     * @warning stating device access group as required via CLI11 functionality does not work as
     * expected. It has a higher precedence than following subcommands, thus it displays the wrong
     * error message. Checking it manually.
     */
    auto require_device_group(const device_access_options &options) -> bool
    {
        if (options.index->count() == 0 && options.serial->count() == 0)
        {
            std::cerr << "[Option Group: --device-at-index or --device-with-serial] is "
                         "required."
                      << std::endl
                      << "Run with --help for more information." << std::endl;
            return false;
        }

        return true;
    }

    /**
     * @brief applies the optional --include-device-types selection of a subcommand onto the given
     * filter, leaving the default untouched when the option was not provided.
     *
     * @attention returns false when the value is malformed; parse_device_types reports the
     * diagnostic.
     */
    auto apply_device_type_selection(const CLI::Option *types_option,
        const std::string &raw_value, sokketter::device_filter &filter) -> bool
    {
        if (types_option->count() == 0)
        {
            return true;
        }

        sokketter::power_strip_type selected_types = sokketter::power_strip_type::UNKNOWN;
        if (!parse_device_types(raw_value, selected_types))
        {
            return false;
        }

        filter.included_types = selected_types;
        return true;
    }

    /**
     * @brief named alternative to the positional bool in resolve_device: whether the
     * --include-device-types selection restricts the devices the subcommand may address.
     */
    enum class restrict_selection
    {
        no,
        yes
    };

    /**
     * @brief resolves the device addressed by exactly one of the device-addressing options,
     * printing the standard diagnostics and returning nullptr when no device answers.
     *
     * @attention shared by the power, login and logout subcommands so that index resolution,
     * serial lookup and type restriction behavior are identical for every subcommand.
     * @attention --device-at-index is 1-based and resolves positionally against the same filtered
     * vector the list subcommand prints (with its default filter, or with the selection given via
     * --include-device-types), so index N always addresses the device printed as N by the matching
     * list invocation. The library's sokketter::device(index) indexes the unfiltered database and
     * must not be used here.
     * @attention visible_devices() performs the enumeration itself on the index path; the serial
     * path consults the persisted database, so it enumerates first to bind live communication
     * handles.
     * @attention when --include-device-types restricts the selection, a serial resolving to a
     * device outside it is reported as not found, mirroring how the index path cannot reach such
     * devices. TEST_DEVICE stays addressable regardless of the selection.
     */
    auto resolve_device(const device_access_options &options, const size_t &device_index,
        const std::string &device_serial, const sokketter::device_filter &filter,
        restrict_selection types_restricted) -> std::shared_ptr<sokketter::power_strip>
    {
        std::shared_ptr<sokketter::power_strip> device;

        if (options.index->count() > 0)
        {
            if (device_index == 0)
            {
                std::cerr << "Device index 0 is out of range (valid range starts from 1)."
                          << std::endl;
                return nullptr;
            }

            const auto visible = visible_devices(filter);

            if (visible.empty())
            {
                std::cerr << "Device index " << device_index
                          << " is out of range (no devices are available)." << std::endl;
                return nullptr;
            }

            if (device_index > visible.size())
            {
                std::cerr << "Device index " << device_index << " is out of range (valid range: 1-"
                          << visible.size() << ")." << std::endl;
                return nullptr;
            }

            device = visible[device_index - 1];
        }
        else if (options.serial->count() > 0)
        {
            sokketter::devices(filter);
            device = sokketter::device(device_serial);

            if (device != nullptr && types_restricted == restrict_selection::yes &&
                device->configuration().type != sokketter::power_strip_type::TEST_DEVICE &&
                !type_matches(device->configuration().type, filter.included_types))
            {
                device = nullptr;
            }
        }

        if (device == nullptr)
        {
            std::cerr << "No device was found for "
                      << (options.index->count() > 0
                                 ? "index " + std::to_string(device_index) + "."
                                 : "serial \"" + device_serial + "\".")
                      << std::endl;
        }

        return device;
    }
} // namespace

auto cli_parser::parse_and_process(int argc, char *argv[]) -> int
{
    /**
     * @brief guard against an empty argument vector (possible via execve on POSIX). Without it,
     * normalized_argv would be empty and its data() may be null; CLI11 never dereferences it today,
     * but the guard keeps the call safe if it is ever changed to parse(normalized_argv).
     */
    if (argc == 0)
    {
        return EXIT_FAILURE;
    }

    /** ************************************************************************
     *
     * @brief parameter configuration section.
     *
     ** ***********************************************************************/

    /**
     * @brief basic CLI11 application configuration.
     */
    CLI::App application;

    application.name("sokketter-cli");
    application.ignore_case();
    application.ignore_underscore();
    application.allow_windows_style_options();
    application.formatter(std::make_shared<overridden_help_formatter>());
    application.require_subcommand(1, 1);

    /**
     * @brief adding a version flag.
     */
    application.set_version_flag(
        "--version,-v", std::string("sokketter-cli version ") + sokketter::version().to_string());

    /**
     * @brief adding a list subcommand.
     */
    auto subcommand_list = application.add_subcommand("list");
    subcommand_list->ignore_underscore();

    /**
     * @brief adding a power subcommand.
     */
    auto subcommand_power = application.add_subcommand("power");
    subcommand_power->ignore_underscore();
    subcommand_power->require_subcommand(1, 1);

    auto subcommand_power_status = subcommand_power->add_subcommand("status");
    auto subcommand_power_on = subcommand_power->add_subcommand("on");
    auto subcommand_power_off = subcommand_power->add_subcommand("off");
    auto subcommand_power_toggle = subcommand_power->add_subcommand("toggle");

    /**
     * @brief adding login and logout subcommands.
     *
     * @attention login authenticates a device with credentials provided on the command line and
     * persists them on success; logout removes the stored credentials of a device that has an
     * authentication mechanism.
     */
    auto subcommand_login = application.add_subcommand("login");
    subcommand_login->ignore_underscore();

    auto subcommand_logout = application.add_subcommand("logout");
    subcommand_logout->ignore_underscore();

    /**
     * @brief adding device and socket access options.
     */
    std::vector<size_t> socket_indices;
    subcommand_power->add_option("--sockets,-s", socket_indices)->ignore_underscore();

    /**
     * @brief adding the device-addressing option group to the power, login and logout
     * subcommands.
     *
     * @attention the subcommands are mutually exclusive (require_subcommand(1, 1)), so a single
     * shared storage backs the index and serial options of every subcommand; presence is tracked
     * per option pointer via the returned device_access_options.
     */
    size_t device_index = 0;
    std::string device_serial;

    const auto power_device_options =
        add_device_options(*subcommand_power, device_index, device_serial);
    const auto login_device_options =
        add_device_options(*subcommand_login, device_index, device_serial);
    const auto logout_device_options =
        add_device_options(*subcommand_logout, device_index, device_serial);

    /**
     * @attention overwriting the default help to show the same text for all subcommands.
     */
    const auto commands = {subcommand_list, subcommand_power, subcommand_power_status,
        subcommand_power_on, subcommand_power_off, subcommand_power_toggle, subcommand_login,
        subcommand_logout};
    for (const auto &command : commands)
    {
        command->set_help_flag();
        command->set_help_all_flag();
        command->fallthrough();
    }

    /**
     * @brief adding an option to select which device types (USB/ETHERNET) are included by the
     * list, power, login and logout subcommands.
     *
     * @attention the options share one storage since the subcommands are mutually exclusive
     * (require_subcommand(1, 1)); presence is tracked per option via the returned pointers.
     */
    std::string included_device_types;
    auto option_list_device_types =
        subcommand_list->add_option("--include-device-types,-t", included_device_types);
    option_list_device_types->ignore_underscore();

    auto option_power_device_types =
        subcommand_power->add_option("--include-device-types,-t", included_device_types);
    option_power_device_types->ignore_underscore();

    auto option_login_device_types =
        subcommand_login->add_option("--include-device-types,-t", included_device_types);
    option_login_device_types->ignore_underscore();

    auto option_logout_device_types =
        subcommand_logout->add_option("--include-device-types,-t", included_device_types);
    option_logout_device_types->ignore_underscore();

    /**
     * @brief adding authentication options to the login subcommand.
     *
     * @attention the options are intentionally not registered on the logout subcommand: logout
     * only clears the stored credentials, so any authentication material supplied to it is
     * rejected by CLI11 as an unexpected option.
     */
    std::string authentication_type;
    std::string authentication_password;

    auto option_authentication_type =
        subcommand_login->add_option("--authentication-type", authentication_type);
    option_authentication_type->ignore_underscore();

    auto option_authentication_password =
        subcommand_login->add_option("--password", authentication_password);
    option_authentication_password->ignore_underscore();

    /** ************************************************************************
     *
     * @brief parameter parsing section.
     *
     ** ***********************************************************************/
    std::vector<std::string> normalized_arguments;
    normalized_arguments.reserve(argc);
    normalized_arguments.emplace_back(argv[0]); // program name: never normalized
    for (int index = 1; index < argc; ++index)
    {
        normalized_arguments.emplace_back(normalize_cli_argument(argv[index]));
    }

    std::vector<char *> normalized_argv;
    normalized_argv.reserve(argc);
    for (auto &argument : normalized_arguments)
    {
        normalized_argv.push_back(argument.data());
    }

    try
    {
        application.parse(argc, normalized_argv.data());
    }
    catch (const CLI::ParseError &e)
    {
        /**
         * @attention application.exit() prints the error/usage text and returns CLI11's own
         * exit code (100+ for parse errors, 0 for the success family: --help/--version).
         * Collapse it onto the documented contract: 0 for success, 1 for any failure.
         */
        const int cli_exit_code = application.exit(e);
        return cli_exit_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /** ************************************************************************
     *
     * @brief update information section.
     *
     * @attention the update notice and the background update check are side effects that must not
     * run for a doomed invocation (e.g. an invalid --include-device-types value), which would pay
     * a network request and interleave the notice before the command's own error on stderr. The
     * work is therefore deferred into this lambda and invoked only once each subcommand's input has
     * been validated and the command is known to be viable.
     *
     ** ***********************************************************************/
    const auto run_update_check = []() {
        const auto update_status = sokketter::last_update_check_status();
        if (!update_status.new_version.empty())
        {
            std::cerr << "A new sokketter version " << update_status.new_version
                      << " is available at " << sokketter::release_link() << "." << std::endl;
        }

        sokketter::check_for_update_async();
    };

    /** ************************************************************************
     *
     * @brief list processing section.
     *
     ** ***********************************************************************/
    if (subcommand_list->parsed())
    {
        std::cout << "Listing available devices..." << std::endl;

        auto filter = default_display_filter();

        if (option_list_device_types->count() > 0)
        {
            /**
             * @attention the option is selective: the listed device types replace the default
             * USB-only filter, so --include-device-types ethernet lists only Ethernet devices.
             */
            sokketter::power_strip_type selected_types = sokketter::power_strip_type::UNKNOWN;
            if (!parse_device_types(included_device_types, selected_types))
            {
                return EXIT_FAILURE;
            }

            filter.included_types = selected_types;
        }

        run_update_check();

        const auto devices = visible_devices(filter);
        if (devices.empty())
        {
            /**
             * @attention an empty device bus is a valid answer to "list", not a command failure, so
             * the subcommand exits successfully. The message is part of the command's answer and is
             * therefore written to stdout alongside the rest of the list output. Scripts that need
             * to distinguish "no devices" from an error should check for the "No devices found."
             * message rather than the exit code.
             */
            std::cout << "No devices found." << std::endl;
            return EXIT_SUCCESS;
        }

        std::cout << "Available devices (indices start from 1):" << std::endl;

        size_t counter = 1;
        for (const auto &device : devices)
        {
            std::cout << counter << ". " << device->to_string() << std::endl;
            counter++;
        }

        return EXIT_SUCCESS;
    }

    /** ************************************************************************
     *
     * @brief power processing section.
     *
     ** ***********************************************************************/
    else if (subcommand_power->parsed())
    {
        if (!require_device_group(power_device_options))
        {
            return EXIT_FAILURE;
        }

        /**
         * @attention --include-device-types restricts the device types that --device-at-index
         * resolves against and that --device-with-serial may match, mirroring the list
         * subcommand: with a selection, index N addresses the device that
         * "list --include-device-types <same selection>" prints as N.
         */
        auto filter = default_display_filter();
        if (!apply_device_type_selection(option_power_device_types, included_device_types, filter))
        {
            return EXIT_FAILURE;
        }

        const auto device = resolve_device(power_device_options, device_index, device_serial,
            filter, option_power_device_types->count() > 0 ? restrict_selection::yes
                                                            : restrict_selection::no);

        if (device == nullptr)
        {
            return EXIT_FAILURE;
        }

        /**
         * @attention track whether every power operation succeeded, so the exit code reflects
         * failures even when other sockets were processed successfully.
         */
        bool all_succeeded = true;

        /**
         * @attention the requested action is invariant across sockets; resolve it once here so the
         * per-socket lambda contains a single decision point instead of re-testing every
         * subcommand on each call.
         */
        enum class power_action
        {
            status,
            on,
            off,
            toggle
        };

        const power_action action = subcommand_power_status->parsed() ? power_action::status
                                    : subcommand_power_on->parsed()   ? power_action::on
                                    : subcommand_power_off->parsed()  ? power_action::off
                                    : subcommand_power_toggle->parsed()
                                                                      ? power_action::toggle
                                    : /* require_subcommand(1, 1) guarantees exactly one action is
                                          parsed; a new action subcommand must be added to this
                                          chain. */
                                      power_action::toggle;

        /**
         * @attention apply the requested power action to a single socket, printing the
         * per-socket result. Returns false if the requested action failed.
         */
        auto apply_action = [&](const sokketter::socket &socket,
                                const size_t display_index) -> bool {
            switch (action)
            {
            case power_action::status: {
                std::cout << "  Socket " << display_index << ": " << socket.to_string()
                          << std::endl;
                return true;
            }

            case power_action::on: {
                if (!socket.power(true))
                {
                    std::cerr << "  Socket " << display_index << ": failed to turn on."
                              << std::endl;
                    return false;
                }
                std::cout << "  Socket " << display_index << ": turned on." << std::endl;
                return true;
            }

            case power_action::off: {
                if (!socket.power(false))
                {
                    std::cerr << "  Socket " << display_index << ": failed to turn off."
                              << std::endl;
                    return false;
                }
                std::cout << "  Socket " << display_index << ": turned off." << std::endl;
                return true;
            }

            case power_action::toggle: {
                if (!socket.toggle())
                {
                    std::cerr << "  Socket " << display_index << ": failed to toggle." << std::endl;
                    return false;
                }
                std::cout << "  Socket " << display_index << ": toggled." << std::endl;
                return true;
            }
            }

            // LCOV_EXCL_START
            return false;
            // LCOV_EXCL_STOP
        };

        auto &sockets = device->sockets();

        if (sockets.empty())
        {
            std::cerr << "Device has no controllable sockets (is it connected?)." << std::endl;
            return EXIT_FAILURE;
        }

        /**
         * @attention build the list of 1-based socket indices to act on once: all sockets when no
         * indices were specified, otherwise the validated and deduplicated user-provided indices.
         */
        std::vector<size_t> targets;

        if (socket_indices.empty())
        {
            targets.resize(sockets.size());
            std::iota(targets.begin(), targets.end(), 1);
        }
        else
        {
            /**
             * @attention validate all indices before touching hardware, so an invalid index cannot
             * leave the device in a partially-changed state.
             */
            for (const auto &socket_index : socket_indices)
            {
                if (socket_index == 0 || socket_index > sockets.size())
                {
                    std::cerr << "Socket index " << socket_index
                              << " is out of range (valid range: 1-" << sockets.size() << ")."
                              << std::endl;
                    return EXIT_FAILURE;
                }
            }

            /**
             * @attention deduplicate the indices after validation, so a repeated index such as
             * --sockets 1 1 applies the action to the socket only once.
             */
            std::sort(socket_indices.begin(), socket_indices.end());
            socket_indices.erase(
                std::unique(socket_indices.begin(), socket_indices.end()), socket_indices.end());

            targets = socket_indices;
        }

        run_update_check();

        std::cout << device->to_string() << std::endl;

        for (const auto target : targets)
        {
            /**
             * @attention decrement CLI socket index to match the vector index.
             */
            if (!apply_action(sockets.at(target - 1), target))
            {
                all_succeeded = false;
            }
        }

        return all_succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    /** ************************************************************************
     *
     * @brief login processing section.
     *
     ** ***********************************************************************/
    else if (subcommand_login->parsed())
    {
        if (!require_device_group(login_device_options))
        {
            return EXIT_FAILURE;
        }

        auto filter = default_display_filter();
        if (!apply_device_type_selection(option_login_device_types, included_device_types, filter))
        {
            return EXIT_FAILURE;
        }

        /**
         * @attention the authentication options are validated before the device is resolved, so
         * malformed credentials input is reported even when no matching device is attached.
         *
         * @attention --authentication-type is required even though it must match the device's
         * advertised mechanism: it forces the caller to name the mechanism it believes it is
         * speaking, so a device that turns out to require something else is reported instead of
         * silently mis-handled once a second mechanism exists.
         */
        if (option_authentication_type->count() == 0)
        {
            std::cerr << "The --authentication-type option is required for the login subcommand."
                      << authentication_type_error_suffix() << std::endl;
            return EXIT_FAILURE;
        }

        sokketter::power_strip_authentication_type requested_type =
            sokketter::power_strip_authentication_type::UNKNOWN;
        if (!parse_authentication_type(authentication_type, requested_type))
        {
            return EXIT_FAILURE;
        }

        if (requested_type == sokketter::power_strip_authentication_type::PASSWORD_ONLY &&
            option_authentication_password->count() == 0)
        {
            std::cerr << "The --password option is required for the PASSWORD authentication type."
                      << std::endl;
            return EXIT_FAILURE;
        }

        const auto device = resolve_device(login_device_options, device_index, device_serial,
            filter, option_login_device_types->count() > 0 ? restrict_selection::yes
                                                            : restrict_selection::no);

        if (device == nullptr)
        {
            return EXIT_FAILURE;
        }

        // one snapshot, reused for both the check and the update below
        const auto configuration = device->configuration();

        if (configuration.authentication.type != requested_type)
        {
            /**
             * @attention wording deliberately differs from the logout diagnostic, which reports the
             * genuinely no-authentication case: this branch also covers a device that requires
             * authentication of a type the CLI cannot speak.
             */
            std::cerr << "The device's authentication mechanism ("
                      << sokketter::power_strip_authentication_type_to_string(
                             configuration.authentication.type)
                      << ") does not match --authentication-type " << authentication_type << "."
                      << std::endl;
            return EXIT_FAILURE;
        }

        run_update_check();

        std::cout << device->to_string() << std::endl;

        if (!device->is_connected())
        {
            std::cerr << "Device is not reachable (is it connected?)." << std::endl;
            return EXIT_FAILURE;
        }

        /**
         * @attention the candidate credentials are applied, tested, and rolled back on failure: the
         * device object is the one held by the library database, and deinitialize() persists that
         * database, so a rejected password must not survive this subcommand in memory either.
         */
        auto candidate_configuration = configuration;
        // authentication.type is owned by the library: the pre-check above already proved the
        // device advertises requested_type, so only the secret is supplied here.
        candidate_configuration.authentication.password = authentication_password;
        device->configure(candidate_configuration);

        if (!device->try_authenticate())
        {
            /**
             * @attention re-read rather than reuse the pre-call snapshot: only the secret supplied
             * by this invocation is rolled back, so a concurrent library-side update to any other
             * configuration field survives.
             */
            auto rollback_configuration = device->configuration();
            rollback_configuration.authentication.password = configuration.authentication.password;
            device->configure(rollback_configuration);

            std::cerr << "Authentication failed: the device rejected the credentials." << std::endl;
            return EXIT_FAILURE;
        }

        /**
         * @attention the success message below asserts persistence, but save() cannot report a
         * failed write (void today; see the @todo on power_strip::save()): with an unwritable
         * storage path the credentials survive only in memory and the message is optimistic.
         */
        device->save();

        std::cout << "Authenticated successfully." << std::endl;
        return EXIT_SUCCESS;
    }

    /** ************************************************************************
     *
     * @brief logout processing section.
     *
     * @attention logout clears the stored credentials of the device configuration and persists
     * the change; it does not terminate a session, since the library opens and closes an
     * authenticated session per operation.
     *
     ** ***********************************************************************/
    else if (subcommand_logout->parsed())
    {
        if (!require_device_group(logout_device_options))
        {
            return EXIT_FAILURE;
        }

        auto filter = default_display_filter();
        if (!apply_device_type_selection(option_logout_device_types, included_device_types, filter))
        {
            return EXIT_FAILURE;
        }

        const auto device = resolve_device(logout_device_options, device_index, device_serial,
            filter, option_logout_device_types->count() > 0 ? restrict_selection::yes
                                                             : restrict_selection::no);

        if (device == nullptr)
        {
            return EXIT_FAILURE;
        }

        // one snapshot, reused for both the checks and the update below
        const auto configuration = device->configuration();

        if (configuration.authentication.type == sokketter::power_strip_authentication_type::NONE ||
            configuration.authentication.type ==
                sokketter::power_strip_authentication_type::UNKNOWN)
        {
            /**
             * @attention deliberate: a device advertising no mechanism, or one whose mechanism
             * the library could not identify, has nothing this subcommand can revoke; a password
             * left behind in its stored record is legacy data and stays out of reach until the
             * device re-advertises authentication.
             */
            std::cerr << "Device does not require authentication." << std::endl;
            return EXIT_FAILURE;
        }

        if (!configuration.authentication.is_valid())
        {
            /**
             * @attention deliberate: logout reports a no-op as a failure, so "credentials are
             * gone" is never reported for a device that never had them. Idempotent teardown must
             * be expressed by the caller checking the message, not the exit code.
             */
            std::cerr << "No credentials are stored for this device." << std::endl;
            return EXIT_FAILURE;
        }

        run_update_check();

        std::cout << device->to_string() << std::endl;

        /**
         * @attention re-read rather than reuse the pre-call snapshot: only the field the CLI owns
         * is cleared, so a concurrent library-side update to any other configuration field
         * survives.
         */
        auto new_configuration = device->configuration();
        new_configuration.authentication.password.clear();
        device->configure(new_configuration);
        /**
         * @attention same caveat as login: save() cannot report a failed write, so this message
         * asserts persistence the library does not yet guarantee (see the @todo on
         * power_strip::save()).
         */
        device->save();

        std::cout << "Stored credentials removed." << std::endl;
        return EXIT_SUCCESS;
    }

    // LCOV_EXCL_START
    return EXIT_FAILURE;
    // LCOV_EXCL_STOP
}
