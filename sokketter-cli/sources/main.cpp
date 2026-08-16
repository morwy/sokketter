#include "cli_parser.h"

#include "libsokketter.h"

#include <iostream>
#include <string>

auto main(int argc, char *argv[]) -> int
{
    sokketter::initialize();

    std::string latest_version;
    if (sokketter::is_new_release_available(latest_version))
    {
        std::cerr << "A new sokketter version " << latest_version << " is available at "
                  << sokketter::release_link() << "." << std::endl;
    }

    const int return_code = cli_parser::parse_and_process(argc, argv);

    sokketter::deinitialize();

    return return_code;
}
