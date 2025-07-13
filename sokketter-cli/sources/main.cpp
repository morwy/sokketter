#include "cli_parser.h"

#include "libsokketter.h"

auto main(int argc, char *argv[]) -> int
{
    sokketter::initialize();

    const int return_code = cli_parser::parse_and_process(argc, argv);

    sokketter::deinitialize();

    return return_code;
}
