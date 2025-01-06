#include <cstdio>
#include <cstdlib>

#include "libsokketter.h"

auto main(int /*argc*/, char * /*argv*/[]) -> int
{
    printf("libsokketter version: %s.\n\n", sokketter::version().to_string().c_str());

    const auto &devices = sokketter::devices();
    printf("Found %zu device(s):\n", devices.size());

    for (const auto &device : devices)
    {
        printf("  - %s.\n", device->to_string().c_str());
    }

    return EXIT_SUCCESS;
}
