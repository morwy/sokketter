#ifndef TEST_DEVICE_H
#define TEST_DEVICE_H

#pragma once

#include <libsokketter.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

#include <mutex>
#include <optional>

class test_device : public sokketter::power_strip
{
public:
    test_device(const size_t &index);
    ~test_device();

    [[nodiscard]] auto is_connected() const -> bool override;

    [[nodiscard]] auto configuration() const -> sokketter::power_strip_configuration override;

    auto configure(const sokketter::power_strip_configuration &configuration) -> void override;

    [[nodiscard]] auto socket(const size_t &index)
        -> const std::optional<std::reference_wrapper<sokketter::socket>> override;

private:
    /**
     * @brief serializes the device configuration, mirroring power_strip_base's state mutex so
     * that the thread-safety contract documented on sokketter::power_strip holds for test
     * devices as well (they are read from the UI worker pool while the user may edit them).
     */
    mutable std::mutex m_state_mutex;

    size_t m_index = 0;
    std::string m_serial_number = "TEST_SERIAL_NUMBER";
    size_t m_socket_number = 4;

    auto power_socket(size_t index, bool is_toggled) -> bool;
    auto socket_status(size_t index) -> bool;
};

#endif // TEST_DEVICE_H
