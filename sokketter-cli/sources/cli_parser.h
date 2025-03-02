#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#pragma once

#include <cstdlib>
#include <sstream>
#include <string>

#include <cli11/CLI11.hpp>

class OverriddenHelpFormatter : public CLI::Formatter
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
        help << "  sokketter-cli list" << std::endl;
        help << "  sokketter-cli power on --sockets 1 --device-at-index 0" << std::endl;
        help << "  sokketter-cli power status --device-with-serial 01:01:60:35:c6" << std::endl;
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
