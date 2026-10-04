#include "cli_parser.h"

#include "libsokketter.h"

#include <algorithm>
#include <cctype>
#include <type_traits>
#include <vector>

namespace {
    /**
     * @brief rewrites underscores to dashes in long-option tokens, e.g. --device_at_index becomes
     * --device-at-index.
     *
     * @attention CLI11's ignore_underscore() strips underscores from both the registered name and
     * the input, so it cannot alias an underscored spelling to a dashed option name. This pass
     * provides that aliasing. It is deliberately limited to tokens that begin with "--" so that
     * option values (paths such as /dev/ttyUSB_0, device serials, or negative numbers) are never
     * silently mutated.
     */
    auto normalize_cli_argument(std::string argument) -> std::string
    {
        if (argument.rfind("--", 0) == 0)
        {
            std::replace(argument.begin(), argument.end(), '_', '-');
        }

        return argument;
    }
} // namespace

auto cli_parser::parse_and_process(int argc, char *argv[]) -> int
{
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

    auto subcommand_power_status = subcommand_power->add_subcommand("status");
    auto subcommand_power_on = subcommand_power->add_subcommand("on");
    auto subcommand_power_off = subcommand_power->add_subcommand("off");
    auto subcommand_power_toggle = subcommand_power->add_subcommand("toggle");

    subcommand_list->excludes(subcommand_power);
    subcommand_power->excludes(subcommand_list);

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

    std::string device_serial = "";
    auto option_device_serial = device_group->add_option("--device-with-serial,-n", device_serial);

    option_device_index->ignore_underscore();
    option_device_serial->ignore_underscore();
    option_device_index->excludes(option_device_serial);
    option_device_serial->excludes(option_device_index);

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
     * @brief adding an option to select which device types (USB/ETHERNET) are included in the list subcommand.
     */
    std::string included_device_types = "";
    auto option_included_devices_types =
        subcommand_list->add_option("--include-device-types,-t", included_device_types);

    /** ************************************************************************
     *
     * @brief parameter parsing section.
     *
     ** ***********************************************************************/
    std::vector<std::string> normalized_arguments;
    normalized_arguments.reserve(argc);
    for (int index = 0; index < argc; ++index)
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
        return application.exit(e);
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

        sokketter::device_filter filter;

        filter.included_types = sokketter::power_strip_type::USB_DEVICES;

        if (option_included_devices_types->count() > 0)
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
                token.erase(std::remove_if(token.begin(), token.end(),
                              [](unsigned char character) { return std::isspace(character) != 0; }),
                    token.end());

                std::transform(token.begin(), token.end(), token.begin(),
                    [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    });

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

        const auto &devices = sokketter::devices(filter);
        if (devices.empty())
        {
            std::cerr << "No devices found." << std::endl;
            return EXIT_FAILURE;
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
         * @warning --help flag is not being forwarded to application level,
         * even though a fallthrough is set for all subcommands.
         * Checking it manually for power and power related subcommands.
         */
        if (subcommand_power->count("--help") > 0 || subcommand_power->count("-h") > 0)
        {
            std::cout << application.help() << std::endl;
            return EXIT_SUCCESS;
        }

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
            /**
             * @attention --device-at-index is 1-based, matching the numbering printed by the list
             * subcommand, so it has to be decremented before being passed to the 0-based library.
             */
            if (device_index == 0)
            {
                std::cerr << "Device index 0 is out of range (valid range starts from 1)."
                          << std::endl;
                return EXIT_FAILURE;
            }

            device = sokketter::device(device_index - 1);
        }
        else if (option_device_serial->count() > 0)
        {
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
         * @attention apply the requested power action to a single socket, printing the
         * per-socket result. Returns false if any of the requested actions failed.
         */
        auto apply_action = [&](const sokketter::socket &socket, const size_t display_index) -> bool
        {
            if (subcommand_power_status->parsed())
            {
                std::cout << "  Socket " << display_index << ": " << socket.to_string() << std::endl;
                return true;
            }

            bool succeeded = true;

            if (subcommand_power_on->parsed())
            {
                if (!socket.power(true))
                {
                    std::cerr << "  Socket " << display_index << ": failed to turn on." << std::endl;
                    succeeded = false;
                }
                else
                {
                    std::cout << "  Socket " << display_index << ": turned on." << std::endl;
                }
            }

            if (subcommand_power_off->parsed())
            {
                if (!socket.power(false))
                {
                    std::cerr << "  Socket " << display_index << ": failed to turn off."
                              << std::endl;
                    succeeded = false;
                }
                else
                {
                    std::cout << "  Socket " << display_index << ": turned off." << std::endl;
                }
            }

            if (subcommand_power_toggle->parsed())
            {
                if (!socket.toggle())
                {
                    std::cerr << "  Socket " << display_index << ": failed to toggle." << std::endl;
                    succeeded = false;
                }
                else
                {
                    std::cout << "  Socket " << display_index << ": toggled." << std::endl;
                }
            }

            return succeeded;
        };

        /**
         * @attention use all sockets if no indices were specified.
         */
        if (sockets_argument->count() == 0 || socket_indices.empty())
        {
            std::cout << device->to_string() << std::endl;

            size_t socket_index = 1;
            for (const auto &socket : device->sockets())
            {
                if (!apply_action(socket, socket_index))
                {
                    all_succeeded = false;
                }

                ++socket_index;
            }
        }

        if (!socket_indices.empty())
        {
            std::cout << device->to_string() << std::endl;
        }

        /**
         * @attention validate all indices before touching hardware, so an invalid index cannot
         * leave the device in a partially-changed state.
         */
        for (const auto &socket_index : socket_indices)
        {
            if (socket_index == 0 || socket_index > device->sockets().size())
            {
                std::cerr << "Socket index " << socket_index << " is out of range (valid range: 1-"
                          << device->sockets().size() << ")." << std::endl;
                return EXIT_FAILURE;
            }
        }

        for (const auto &socket_index : socket_indices)
        {
            /**
             * @attention decrement CLI socket index to match the vector index.
             */
            const auto &socket = device->sockets().at(socket_index - 1);

            if (!apply_action(socket, socket_index))
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
