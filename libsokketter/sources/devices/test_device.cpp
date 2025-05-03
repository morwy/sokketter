#include "test_device.h"

#include <map>

static std::map<std::string, std::vector<sokketter::socket>> gs_sockets;
static std::map<std::string, std::vector<bool>> gs_socket_states;

test_device::test_device(const size_t &index)
    : m_index(index)
{
    m_serial_number = "TEST_SERIAL_NUMBER_" + std::to_string(m_index);

    auto basic_configuration = configuration();
    basic_configuration.id = m_serial_number;
    basic_configuration.type = sokketter::power_strip_type::TEST_DEVICE;
    basic_configuration.name = "Test Device " + std::to_string(m_index);
    basic_configuration.address = "TEST_ADDRESS_" + std::to_string(m_index);
    configure(basic_configuration);

    if (gs_socket_states[m_serial_number].size() != m_socket_number)
    {
        gs_socket_states[m_serial_number].resize(m_socket_number, false);
    }

    gs_sockets[m_serial_number].clear();

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(
                &test_device::power_socket, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&test_device::socket_status, this, std::placeholders::_1));
        gs_sockets[m_serial_number].push_back(socket);
    }
}

auto test_device::is_connected() const -> bool
{
    return true;
}

auto test_device::sockets() -> const std::vector<sokketter::socket> &
{
    return gs_sockets[m_serial_number];
}

auto test_device::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    if (index >= gs_sockets[m_serial_number].size())
    {
        return std::nullopt;
    }

    return gs_sockets[m_serial_number][index];
}

auto test_device::power_socket(size_t index, bool is_toggled) -> bool
{
    gs_socket_states[m_serial_number][index - 1] = is_toggled;
    return true;
}

auto test_device::socket_status(size_t index) -> bool
{
    return gs_socket_states[m_serial_number][index - 1];
}
