#include <cstdlib>

#include "libsokketter.h"

#include <cli11/CLI11.hpp>

class CustomFormatter : public CLI::Formatter
{
public:
    std::string make_description(const CLI::App *app) const override
    {
        return CLI::Formatter::make_description(app) + "\n";
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
    application.description(
        "A command-line interface for controlling attached power strips and sockets");
    application.require_subcommand();
    application.formatter(std::make_shared<CustomFormatter>());

    /**
     * @brief adding a version flag.
     */
    auto flag_version = application.add_flag("--version,-v", "Prints the version of sokketter-cli");

    /**
     * @attention overriding default help flags to show help with subcommands.
     */
    application.set_help_flag("");
    application.set_help_all_flag("--help,-h", "Prints descriptive help message and exits");

    /**
     * @brief states short hash of last Git commit
     */
    auto subcommand_list = application.add_subcommand("list", "Lists all available devices");

    /**
     * @brief states short hash of last Git commit
     */
    auto subcommand_power = application.add_subcommand(
        "power", "Contains actions related to power control of the socket(s)");

    auto subcommand_power_status =
        subcommand_power->add_subcommand("status", "States current power state of the socket(s)")
            ->fallthrough();
    auto subcommand_power_on =
        subcommand_power->add_subcommand("on", "Turns power on the socket(s)")->fallthrough();
    auto subcommand_power_off =
        subcommand_power->add_subcommand("off", "Turns power off the socket(s)")->fallthrough();
    auto subcommand_power_toggle =
        subcommand_power->add_subcommand("toggle", "Toggles power state of the socket(s)")
            ->fallthrough();

    /**
     * @brief states short hash of last Git commit
     */
    std::vector<size_t> socket_indices;
    auto sockets_argument = subcommand_power->add_option(
        "--sockets,-s", socket_indices, "States one or multiple socket(s) index");

    auto device_group = subcommand_power->add_option_group("Device selection");

    size_t device_index = 0;
    auto option_device_index = device_group->add_option(
        "--device-by-index,-i", device_index, "States which power strip to use by its index");

    std::string device_serial = "";
    auto option_device_serial = device_group->add_option("--device-by-serial,-n", device_serial,
        "States which power strip to use by its serial number");

    option_device_index->excludes(option_device_serial);
    option_device_serial->excludes(option_device_index);

    /**
     * @brief states short hash of last Git commit
     */
    application.footer("Possible things include betingalw, chiz, and res.");

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
     * @brief version section.
     *
     ** ***********************************************************************/
    if (flag_version->count() > 0)
    {
        std::cout << "sokketter-cli version: " << sokketter::version().to_string() << std::endl;
        return EXIT_SUCCESS;
    }

    /** ************************************************************************
     *
     * @brief list section.
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
     * @brief power section.
     *
     ** ***********************************************************************/
    if (subcommand_power->parsed())
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
        if (socket_indices.empty())
        {
            size_t socket_index = 1;
            for (const auto &socket : device->sockets())
            {
                if (subcommand_power_status->parsed())
                {
                    std::cout << "Socket " << socket_index << ": " << socket.to_string()
                              << std::endl;
                }

                if (subcommand_power_on->parsed())
                {
                    socket.power(true);
                    std::cout << "Socket " << socket_index << ": turned on." << std::endl;
                }

                if (subcommand_power_off->parsed())
                {
                    socket.power(false);
                    std::cout << "Socket " << socket_index << ": turned off." << std::endl;
                }

                if (subcommand_power_toggle->parsed())
                {
                    socket.power(!socket.is_powered_on());
                    std::cout << "Socket " << socket_index << ": toggled." << std::endl;
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

            const auto &socket = device->sockets().at(socket_index);

            if (subcommand_power_status->parsed())
            {
                std::cout << "Socket " << socket_index << ": " << socket.to_string() << std::endl;
            }

            if (subcommand_power_on->parsed())
            {
                socket.power(true);
                std::cout << "Socket " << socket_index << ": turned on." << std::endl;
            }

            if (subcommand_power_off->parsed())
            {
                socket.power(false);
                std::cout << "Socket " << socket_index << ": turned off." << std::endl;
            }

            if (subcommand_power_toggle->parsed())
            {
                socket.power(!socket.is_powered_on());
                std::cout << "Socket " << socket_index << ": toggled." << std::endl;
            }
        }

        return EXIT_SUCCESS;
    }

    /** ************************************************************************
     *
     * @brief no CLI parameters section.
     *
     ** ***********************************************************************/
    std::cerr << "No command line parameters were specified!" << std::endl << std::endl;

    std::cerr << application.help() << std::endl;

    return EXIT_FAILURE;
}
