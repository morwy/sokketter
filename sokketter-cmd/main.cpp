#include <cstdlib>

#include "libsokketter.h"

#include <cli11/CLI11.hpp>
#include <sstream>

class OverriddenHelpFormatter : public CLI::Formatter
{
public:
    std::string make_help(const CLI::App *app, std::string, CLI::AppFormatMode) const override
    {
        std::stringstream help;

        help << "A command-line interface for controlling attached power strips and sockets."
             << std::endl;
        help << std::endl;

        help << "Usage: sokketter-cli SUBCOMMAND [OPTIONS]" << std::endl;
        help << std::endl;

        help << "Options:" << std::endl;
        help << "  -h,--help\tPrints descriptive help message and exits." << std::endl;
        help << "  -v,--version\tPrints the version of sokketter-cli." << std::endl;
        help << std::endl;

        help << "Subcommands:" << std::endl;
        help << "  list\t\tLists all available devices." << std::endl;
        help << std::endl;

        help << "  power\t\tContains actions related to power control of the socket(s)."
             << std::endl;
        help << std::endl;

        help << "  Subcommands:" << std::endl;
        help << "    status\t\tStates current power state of the socket(s)." << std::endl;
        help << "    on\t\tTurns power on the socket(s)." << std::endl;
        help << "    off\t\tTurns power off the socket(s)." << std::endl;
        help << "    toggle\t\tToggles power state of the socket(s)." << std::endl;
        help << std::endl;

        help << "  Options:" << std::endl;
        help << "    -i,--device-at-index UINT\tStates which power strip to use by its index. "
                "Excludes --device-with-serial option."
             << std::endl;
        help << "    -n,--device-with-serial TEXT\tStates which power strip to use by its serial "
                "number. Excludes --device-at-index option."
             << std::endl;
        help << "    -s,--sockets UINT ...\tStates one or multiple socket(s) indices. Empty option "
                "means that subcommand will apply to all available sockets."
             << std::endl;
        help << std::endl;

        help << "Examples:" << std::endl;
        help << "  sokketter-cli power on --sockets 1 --device-at-index 0" << std::endl;
        help << "  sokketter-cli power status --device-with-serial 01:01:60:35:c6" << std::endl;
        help << std::endl;

        return help.str();
    }
};

auto main(int argc, char *argv[]) -> int
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
    application.formatter(std::make_shared<OverriddenHelpFormatter>());

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

    /**
     * @brief adding device and socket access options.
     */
    std::vector<size_t> socket_indices;
    auto sockets_argument = subcommand_power->add_option("--sockets,-s", socket_indices);

    auto device_group = subcommand_power->add_option_group("Device selection");

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
        const auto devices = sokketter::devices();
        if (devices.empty())
        {
            std::cout << "No devices found." << std::endl;
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
        std::unique_ptr<sokketter::power_strip> device;

        /**
         * @attention ensure at least one option is provided
         */
        if (option_device_index->count() == 0 && option_device_serial->count() == 0)
        {
            std::cerr << "Error: You must provide either --device-by-index or --device-by-serial.";
            std::cout << application.help() << std::endl;
            return EXIT_FAILURE;
        }

        if (option_device_index->count() > 0)
        {
            auto tmp_device = sokketter::device(device_index);
            device = std::move(tmp_device);
        }
        else if (option_device_serial->count() > 0)
        {
            auto tmp_device = sokketter::device(device_serial);
            device = std::move(tmp_device);
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

        for (const auto &socket_index : socket_indices)
        {
            if (socket_index >= device->sockets().size())
            {
                std::cerr << "Socket index " << socket_index << " is out of range." << std::endl;
                return EXIT_FAILURE;
            }

            std::cout << device->to_string() << std::endl;

            const auto &socket = device->sockets().at(socket_index);

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

    /** ************************************************************************
     *
     * @brief no CLI parameters section.
     *
     ** ***********************************************************************/
    std::cerr << "Error: list or power subcommand is required!" << std::endl << std::endl;

    std::cerr << application.help() << std::endl;

    return EXIT_FAILURE;
}
