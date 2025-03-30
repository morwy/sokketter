#ifndef TEST_DEVICE_H
#define TEST_DEVICE_H

#pragma once

#include <libsokketter.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

class test_device : public sokketter::power_strip
{
public:
    test_device();

    [[nodiscard]] auto is_connected() const -> bool override;

    [[nodiscard]] auto sockets() -> const std::vector<sokketter::socket> & override;

    [[nodiscard]] auto socket(const size_t &index)
        -> const std::optional<std::reference_wrapper<sokketter::socket>> override;

private:
    std::string m_serial_number = "TEST_SERIAL_NUMBER";
    size_t m_socket_number = 4;

    auto power_socket(size_t index, bool is_toggled) -> bool;
    auto socket_status(size_t index) -> bool;
};

#endif // TEST_DEVICE_H
