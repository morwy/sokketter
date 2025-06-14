#include "test_device.h"

#include <map>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>

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

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: construction.", this->to_string());

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

test_device::~test_device()
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destruction.", this->to_string());
}

auto test_device::is_connected() const -> bool
{
    return true;
}

auto test_device::sockets() const -> const std::vector<sokketter::socket> &
{
    return gs_sockets[m_serial_number];
}

auto test_device::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    if (index >= gs_sockets[m_serial_number].size())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: index {} is out of range 0-{}!",
            this->to_string(), index, gs_sockets[m_serial_number].size());
        return std::nullopt;
    }

    return gs_sockets[m_serial_number][index];
}

auto test_device::power_socket(size_t index, bool is_toggled) -> bool
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");
    gs_socket_states[m_serial_number][index - 1] = is_toggled;
    return true;
}

auto test_device::socket_status(size_t index) -> bool
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);
    return gs_socket_states[m_serial_number][index - 1];
}
