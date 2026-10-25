#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#pragma once

#include <sstream>
#include <string>

#include <cli11/CLI11.hpp>

/**
 * @brief the authentication types accepted by the login subcommand, as the single source of truth
 * for the parser diagnostics, the help text, and the tests.
 */
inline auto supported_authentication_type_names() -> std::string
{
    return "PASSWORD";
}

/**
 * @brief the "Available types are: ..." sentence appended by the authentication-type diagnostics,
 * derived from supported_authentication_type_names().
 */
inline auto authentication_type_error_suffix() -> std::string
{
    return " Available types are: " + supported_authentication_type_names() + ".";
}

class overridden_help_formatter : public CLI::Formatter
{
public:
    auto make_help(const CLI::App *app, std::string, CLI::AppFormatMode) const
        -> std::string override
    {
        std::stringstream help;

        help << "A command-line interface for controlling attached power strips and sockets."
             << std::endl;
        help << std::endl;

        help << "Usage: sokketter-cli SUBCOMMAND [OPTIONS]" << std::endl;
        help << std::endl;

        help << "Options:" << std::endl;
        help << "  -h,--help\tPrints descriptive help message and exits." << std::endl;
        help << "  -v,--version\tPrints the version of sokketter-cli and exits." << std::endl;
        help << std::endl;

        help << "Subcommands:" << std::endl;
        help << "  list\t\tLists all available devices." << std::endl;
        help << std::endl;

        help << "  Options:" << std::endl;
        help << "    -t,--include-device-types TEXT\tStates which device types to include in the "
                "list, replacing the default USB-only selection. Comma-separated, case-insensitive. "
                "Available types are: USB, ETHERNET, LAN. Unknown types are rejected. Default: USB. "
                "Test devices are always listed regardless of the selected types."
             << std::endl;
        help << std::endl;

        help << "  power\t\tContains actions related to power control of the socket(s)."
             << std::endl;
        help << std::endl;

        help << "  Subcommands:" << std::endl;
        help << "    status\tStates current power state of the socket(s)." << std::endl;
        help << "    on\t\tTurns power on the socket(s)." << std::endl;
        help << "    off\t\tTurns power off the socket(s)." << std::endl;
        help << "    toggle\tToggles power state of the socket(s)." << std::endl;
        help << std::endl;

        help << "  Options:" << std::endl;
        help << "    -i,--device-at-index UINT\t\tStates which power strip to use by its index. "
                "Indices start from 1 and match the numbering printed by the list subcommand "
                "with the same --include-device-types selection (default: USB-only). Without "
                "the option, Ethernet devices are addressable only via --device-with-serial. "
                "Excludes --device-with-serial option."
             << std::endl;
        help << "    -n,--device-with-serial TEXT\tStates which power strip to use by its serial "
                "number, as printed by the list\n\t\t\t\t\tsubcommand (the identifier in "
                "parentheses). Excludes --device-at-index option."
             << std::endl;
        help << "    -s,--sockets UINT ...\t\tStates one or multiple socket(s) indices. Omitting "
                "this option applies the\n\t\t\t\t\tsubcommand to all available sockets. Indices "
                "start from 1 to be in accordance with physical markings\n\t\t\t\t\ton the device."
             << std::endl;
        help << "    -t,--include-device-types TEXT\tRestricts which device types the device "
                "selection may address, the same way as for the list subcommand. Comma-separated, "
                "case-insensitive. Available types are: USB, ETHERNET, LAN. Unknown types are "
                "rejected. Default: USB. Test devices are always addressable regardless of the "
                "selected types."
             << std::endl;
        help << std::endl;

        help << "  login\t\tAuthenticates the device with the provided credentials and stores "
                "them on success."
             << std::endl;
        help << std::endl;

        help << "  Options:" << std::endl;
        help << "    -i,--device-at-index UINT\t\tSame as for the power subcommand. "
                "Excludes --device-with-serial option."
             << std::endl;
        help << "    -n,--device-with-serial TEXT\tSame as for the power subcommand. "
                "Excludes --device-at-index option."
             << std::endl;
        help << "    -t,--include-device-types TEXT\tSame as for the power subcommand."
             << std::endl;
        help << "    --authentication-type TEXT\tStates the authentication mechanism of the "
                "device. Case-insensitive. Available types are: "
             << supported_authentication_type_names() << ". Required."
             << std::endl;
        help << "    --password TEXT\t\tThe password used for the PASSWORD authentication "
                "type. Required for that type."
             << std::endl;
        help << "    @warning a password given on the command line is visible to other local "
                "users via the process list and is kept in the shell history. Prefer a dedicated "
                "mechanism where that matters."
             << std::endl;
        help << std::endl;

        help << "  logout\t\tRemoves the stored authentication credentials of the "
                "device."
             << std::endl;
        help << std::endl;

        help << "  Options:" << std::endl;
        help << "    -i,--device-at-index UINT\t\tSame as for the power subcommand. "
                "Excludes --device-with-serial option."
             << std::endl;
        help << "    -n,--device-with-serial TEXT\tSame as for the power subcommand. "
                "Excludes --device-at-index option."
             << std::endl;
        help << "    -t,--include-device-types TEXT\tSame as for the power subcommand."
             << std::endl;
        help << std::endl;

        help << "Exit codes:" << std::endl;
        help << "  0\t\tThe command completed successfully." << std::endl;
        help << "  1\t\tThe command failed." << std::endl;
        help << std::endl;

        help << "Examples:" << std::endl;
        help << "  sokketter-cli list" << std::endl;
        help << "  sokketter-cli power on --sockets 1 --device-at-index 1" << std::endl;
        help << "  sokketter-cli power status --device-with-serial 01:02:03:04:05" << std::endl;
        help << "  sokketter-cli login --device-at-index 1 --authentication-type PASSWORD "
                "--password secret"
             << std::endl;
        help << "  sokketter-cli logout --device-with-serial 01:02:03:04:05" << std::endl;
        help << std::endl;

        return help.str();
    }
};

class cli_parser
{
public:
    static auto parse_and_process(int argc, char *argv[]) -> int;
};

#endif // CLI_PARSER_H
