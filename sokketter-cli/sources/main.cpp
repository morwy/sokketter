#include "cli_parser.h"

#include "libsokketter.h"

#include <iostream>
#include <string>

auto main(int argc, char *argv[]) -> int
{
    sokketter::initialize();

    const auto update_status = sokketter::last_update_check_status();
    if (!update_status.new_version.empty())
    {
        std::cerr << "A new sokketter version " << update_status.new_version << " is available at "
                  << sokketter::release_link() << "." << std::endl;
    }

    sokketter::check_for_update_async();

    const int return_code = cli_parser::parse_and_process(argc, argv);

    sokketter::deinitialize();

    return return_code;
}
