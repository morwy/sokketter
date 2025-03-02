#include "test_device.h"

test_device::test_device()
{
    auto basic_configuration = configuration();
    basic_configuration.id = m_serial_number;
    basic_configuration.type = sokketter::power_strip_type::TEST_DEVICE;
    basic_configuration.name = "Test Device";
    basic_configuration.address = "TEST_ADDRESS";
    configure(basic_configuration);

    m_socket_states.resize(m_socket_number, false);

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(
                &test_device::power_socket, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&test_device::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

auto test_device::is_connected() const -> bool
{
    return true;
}

auto test_device::sockets() -> const std::vector<sokketter::socket> &
{
    return m_sockets;
}

auto test_device::power_socket(size_t index, bool is_toggled) -> bool
{
    return m_socket_states[index - 1] = is_toggled;
}

auto test_device::socket_status(size_t index) -> bool
{
    return m_socket_states[index - 1];
}
