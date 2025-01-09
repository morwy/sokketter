#include <cstdio>
#include <cstdlib>
#include <thread>

#include "libsokketter.h"

auto main(int /*argc*/, char * /*argv*/[]) -> int
{
    printf("libsokketter version: %s.\n\n", sokketter::version().to_string().c_str());

    const auto &devices = sokketter::devices();
    printf("Found %zu device(s):\n", devices.size());

    const auto max_time_between_toggle_msec = 1000;

    for (const auto &device : devices)
    {
        printf("  - %s.\n", device->to_string().c_str());

        const auto &sockets = device->sockets();
        for (size_t socket_index = 0; socket_index < sockets.size(); socket_index++)
        {
            const auto &socket = sockets[socket_index];

            socket.power(false);

            std::this_thread::sleep_for(std::chrono::milliseconds(max_time_between_toggle_msec));

            printf("      %zu: %s.\n", socket_index + 1, socket.to_string().c_str());

            socket.power(true);

            std::this_thread::sleep_for(std::chrono::milliseconds(max_time_between_toggle_msec));

            printf("      %zu: %s.\n", socket_index + 1, socket.to_string().c_str());
        }
    }

    return EXIT_SUCCESS;
}
