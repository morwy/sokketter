#include "cli_parser.h"

#include "libsokketter.h"

int cli_parser::parse_and_process(int argc, char *argv[])
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
    application.formatter(std::make_shared<OverriddenHelpFormatter>());
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

    /**
     * @brief adding a power subcommand.
     */
    auto subcommand_power = application.add_subcommand("power");

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

    auto device_group =
        subcommand_power->add_option_group("--device-at-index or --device-with-serial");

    size_t device_index = 0;
    auto option_device_index = device_group->add_option("--device-at-index,-i", device_index);

    std::string device_serial = "";
    auto option_device_serial = device_group->add_option("--device-with-serial,-n", device_serial);

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

    /** ************************************************************************
     *
     * @brief parameter parsing section.
     *
     ** ***********************************************************************/
    try
    {
        application.parse(argc, argv);
    }
    catch (const CLI::ParseError &e)
    {
        return application.exit(e);
    }

    /** ************************************************************************
     *
     * @brief list processing section.
     *
     ** ***********************************************************************/
    if (subcommand_list->parsed())
    {
        const auto& devices = sokketter::devices();
        if (devices.empty())
        {
            std::cerr << "No devices found." << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "Available devices:" << std::endl;

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
            device = sokketter::device(device_index);
        }
        else if (option_device_serial->count() > 0)
        {
            device = sokketter::device(device_serial);
        }

        if (device == nullptr)
        {
            std::cerr << "No device was found." << std::endl;
            return EXIT_FAILURE;
        }

        /**
         * @attention use all sockets if no indices were specified.
         */
        if (sockets_argument->count() == 0 || socket_indices.empty())
        {
            std::cout << device->to_string() << std::endl;

            size_t socket_index = 1;
            for (const auto &socket : device->sockets())
            {
                if (subcommand_power_status->parsed())
                {
                    std::cout << "  Socket " << socket_index << ": " << socket.to_string()
                              << std::endl;
                }

                if (subcommand_power_on->parsed())
                {
                    socket.power(true);
                    std::cout << "  Socket " << socket_index << ": turned on." << std::endl;
                }

                if (subcommand_power_off->parsed())
                {
                    socket.power(false);
                    std::cout << "  Socket " << socket_index << ": turned off." << std::endl;
                }

                if (subcommand_power_toggle->parsed())
                {
                    socket.power(!socket.is_powered_on());
                    std::cout << "  Socket " << socket_index << ": toggled." << std::endl;
                }

                ++socket_index;
            }
        }

        if (!socket_indices.empty())
        {
            std::cout << device->to_string() << std::endl;
        }

        for (const auto &socket_index : socket_indices)
        {
            if (socket_index == 0 || socket_index > device->sockets().size())
            {
                std::cerr << "Socket index " << socket_index << " is out of range." << std::endl;
                return EXIT_FAILURE;
            }

            /**
             * @attention decrement CLI socket index to match the vector index.
             */
            const auto &socket = device->sockets().at(socket_index - 1);

            if (subcommand_power_status->parsed())
            {
                std::cout << "  Socket " << socket_index << ": " << socket.to_string() << std::endl;
            }

            if (subcommand_power_on->parsed())
            {
                socket.power(true);
                std::cout << "  Socket " << socket_index << ": turned on." << std::endl;
            }

            if (subcommand_power_off->parsed())
            {
                socket.power(false);
                std::cout << "  Socket " << socket_index << ": turned off." << std::endl;
            }

            if (subcommand_power_toggle->parsed())
            {
                socket.power(!socket.is_powered_on());
                std::cout << "  Socket " << socket_index << ": toggled." << std::endl;
            }
        }

        return EXIT_SUCCESS;
    }

    // LCOV_EXCL_START
    return EXIT_FAILURE;
    // LCOV_EXCL_STOP
}
