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
        using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;

        const auto included_types = static_cast<underlying_type>(filter.included_types);
        const auto test_device_type =
            static_cast<underlying_type>(sokketter::power_strip_type::TEST_DEVICE);

        std::vector<std::shared_ptr<sokketter::power_strip>> visible;
        for (const auto &device : sokketter::devices(filter))
        {
            const auto device_type = static_cast<underlying_type>(device->configuration().type);

            if (device_type == test_device_type || (device_type & included_types) != 0)
            {
                visible.push_back(device);
            }
        }
        return visible;
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
     * @brief adding device and socket access options.
     */
    std::vector<size_t> socket_indices;
    auto sockets_argument = subcommand_power->add_option("--sockets,-s", socket_indices);
    sockets_argument->ignore_underscore();

    auto device_group =
        subcommand_power->add_option_group("--device-at-index or --device-with-serial");

    size_t device_index = 0;
    auto option_device_index = device_group->add_option("--device-at-index,-i", device_index);

    std::string device_serial;
    auto option_device_serial = device_group->add_option("--device-with-serial,-n", device_serial);

    option_device_index->ignore_underscore();
    option_device_serial->ignore_underscore();
    option_device_index->excludes(option_device_serial);
    option_device_serial->excludes(option_device_index);

    /**
     * @warning option groups are nameless subcommands and inherit a help flag from the parent.
     * That flag captures --help before it can fall through to the application level, which breaks
     * help precedence (e.g. "power --help" fails the required subcommand check instead of showing
     * help). Removing it lets --help fall through to the application-level help flag.
     */
    device_group->set_help_flag();
    device_group->set_help_all_flag();

    /**
     * @attention overwriting the default help to show the same text for all subcommands.
     */
    const auto commands = {subcommand_list, subcommand_power, subcommand_power_status,
        subcommand_power_on, subcommand_power_off, subcommand_power_toggle};
    for (const auto &command : commands)
    {
        command->set_help_flag();
        command->set_help_all_flag();
        command->fallthrough();
    }

    /**
     * @brief adding an option to select which device types (USB/ETHERNET) are included in the list
     * subcommand.
     */
    std::string included_device_types;
    auto option_included_device_types =
        subcommand_list->add_option("--include-device-types,-t", included_device_types);
    option_included_device_types->ignore_underscore();

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
     * @brief show update information section.
     *
     ** ***********************************************************************/
    const auto update_status = sokketter::last_update_check_status();
    if (!update_status.new_version.empty())
    {
        std::cerr << "A new sokketter version " << update_status.new_version << " is available at "
                  << sokketter::release_link() << "." << std::endl;
    }

    sokketter::check_for_update_async();

    /** ************************************************************************
     *
     * @brief list processing section.
     *
     ** ***********************************************************************/
    if (subcommand_list->parsed())
    {
        std::cout << "Listing available devices..." << std::endl;

        auto filter = default_display_filter();

        if (option_included_device_types->count() > 0)
        {
            /**
             * @attention the option is selective: the listed device types replace the default
             * USB-only filter, so --include-device-types ethernet lists only Ethernet devices.
             * Tokens are split on comma and matched exactly, so unknown or misspelled
             * values are rejected instead of being silently ignored.
             */
            using underlying_type = std::underlying_type_t<sokketter::power_strip_type>;

            underlying_type selected_types = 0;

            std::istringstream token_stream(included_device_types);
            std::string token;
            while (std::getline(token_stream, token, ','))
            {
                token.erase(
                    std::remove_if(token.begin(), token.end(),
                        [](unsigned char character) { return std::isspace(character) != 0; }),
                    token.end());

                std::transform(
                    token.begin(), token.end(), token.begin(), [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    });

                if (token.empty())
                {
                    continue;
                }

                if (token == "usb")
                {
                    selected_types |=
                        static_cast<underlying_type>(sokketter::power_strip_type::USB_DEVICES);
                }
                else if (token == "ethernet" || token == "lan")
                {
                    selected_types |=
                        static_cast<underlying_type>(sokketter::power_strip_type::ETHERNET_DEVICES);
                }
                else
                {
                    std::cerr << "Unknown device type: " << token
                              << ". Available types are: USB, ETHERNET, LAN." << std::endl;
                    return EXIT_FAILURE;
                }
            }

            if (selected_types == 0)
            {
                std::cerr << "No device types were specified. Available types are: USB, ETHERNET, "
                             "LAN."
                          << std::endl;
                return EXIT_FAILURE;
            }

            filter.included_types = static_cast<sokketter::power_strip_type>(selected_types);
        }

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
        /**
         * @warning stating device access group as required via CLI11 functionality
         * does not work as expected. It has a higher precedence than following subcommands,
         * thus it displays the wrong error message. Checking it manually.
         */
        if (option_device_index->count() == 0 && option_device_serial->count() == 0)
        {
            std::cerr << "[Option Group: --device-at-index or --device-with-serial] is "
                         "required."
                      << std::endl
                      << "Run with --help for more information." << std::endl;
            return EXIT_FAILURE;
        }

        std::shared_ptr<sokketter::power_strip> device;

        if (option_device_index->count() > 0)
        {
            if (device_index == 0)
            {
                std::cerr << "Device index 0 is out of range (valid range starts from 1)."
                          << std::endl;
                return EXIT_FAILURE;
            }

            /**
             * @attention --device-at-index is 1-based and resolves positionally against the same
             * filtered vector the list subcommand prints with its default filter, so index N always
             * addresses the device printed as N. The library's sokketter::device(index) indexes the
             * unfiltered database and must not be used here.
             * @attention visible_devices() performs the enumeration itself, so no separate
             * sokketter::devices() call is needed on this path.
             */
            const auto visible = visible_devices(default_display_filter());

            if (visible.empty())
            {
                std::cerr << "Device index " << device_index
                          << " is out of range (no devices are available)." << std::endl;
                return EXIT_FAILURE;
            }

            if (device_index > visible.size())
            {
                std::cerr << "Device index " << device_index << " is out of range (valid range: 1-"
                          << visible.size() << ")." << std::endl;
                return EXIT_FAILURE;
            }

            device = visible[device_index - 1];
        }
        else if (option_device_serial->count() > 0)
        {
            /**
             * @attention serial lookup consults the persisted database, so enumerate first to bind
             * live communication handles.
             */
            sokketter::devices();
            device = sokketter::device(device_serial);
        }

        if (device == nullptr)
        {
            std::cerr << "No device was found for "
                      << (option_device_index->count() > 0
                                 ? "index " + std::to_string(device_index) + "."
                                 : "serial \"" + device_serial + "\".")
                      << std::endl;
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
                                                                      : power_action::toggle;

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

    // LCOV_EXCL_START
    return EXIT_FAILURE;
    // LCOV_EXCL_STOP
}
