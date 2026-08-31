#include "cli_parser.h"

#include "libsokketter.h"

#include <iostream>

namespace {
    /**
     * @brief redirects sokketter and kommpot log output to stderr, keeping stdout free for the
     * command's own output so scripted usage of the CLI can rely on it.
     */
    auto cli_logging_callback(const sokketter::callback_response_structure &response) -> void
    {
        std::cerr << response.message << std::endl;
    }
} // namespace

auto main(int argc, char *argv[]) -> int
{
    sokketter::settings_structure settings;
    settings.logging_callback = cli_logging_callback;
    sokketter::set_settings(settings);

    sokketter::initialize();

    const int return_code = cli_parser::parse_and_process(argc, argv);

    sokketter::deinitialize();

    return return_code;
}
