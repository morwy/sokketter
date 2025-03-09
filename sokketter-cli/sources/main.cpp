#include "cli_parser.h"

auto main(int argc, char *argv[]) -> int
{
    return cli_parser::parse_and_process(argc, argv);
}
