#include "test_device.h"

#include <cstdlib>
#include <map>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>

/**
 * @brief socket on/off states keyed by serial number, so that separate test_device instances
 * created for the same device share the last recorded states within a single process.
 * @attention the socket objects themselves live in the per-instance m_sockets member.
 * @attention guarded by gs_socket_states_mutex: the GUI thread toggles sockets while the UI
 * worker pool reads the states through socket_status().
 */
static std::map<std::string, std::vector<bool>> gs_socket_states;
static std::mutex gs_socket_states_mutex;

test_device::test_device(const size_t &index)
    : m_index(index)
{
    m_serial_number = "TEST_SERIAL_NUMBER_" + std::to_string(m_index);

    auto basic_configuration = configuration();
    basic_configuration.id = m_serial_number;
    basic_configuration.type = sokketter::power_strip_type::TEST_DEVICE;
    basic_configuration.name = "Test Device " + std::to_string(m_index);
    basic_configuration.address = "TEST_ADDRESS_" + std::to_string(m_index);

    /**
     * @attention opt-in fake authentication: when LIBSOKKETTER_TEST_DEVICE_PASSWORD is set, the
     * device advertises PASSWORD_ONLY authentication and try_authenticate() validates against the
     * variable's value, so authentication flows (login/logout) can be exercised without real
     * hardware. The variable is re-read on every construction, letting tests change the behavior
     * between enumerations within one process.
     */
    const char *authentication_password = std::getenv("LIBSOKKETTER_TEST_DEVICE_PASSWORD");
    if (authentication_password != nullptr)
    {
        m_authentication_password = authentication_password;
        basic_configuration.authentication.type =
            sokketter::power_strip_authentication_type::PASSWORD_ONLY;
    }

    configure(basic_configuration);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: construction.", this->to_string());

    {
        const std::lock_guard<std::mutex> states_lock(gs_socket_states_mutex);

        if (gs_socket_states[m_serial_number].size() != m_socket_number)
        {
            gs_socket_states[m_serial_number].resize(m_socket_number, false);
        }
    }

    m_sockets.clear();
    m_sockets.reserve(m_socket_number);

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        m_sockets.emplace_back(socket_index,
            std::bind(
                &test_device::power_socket, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&test_device::socket_status, this, std::placeholders::_1));
    }
}

test_device::~test_device()
{
    if (SOKKETTER_LOGGER == nullptr)
    {
        return;
    }

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destruction.", this->to_string());
}

auto test_device::is_connected() const -> bool
{
    return true;
}

auto test_device::configuration() const -> sokketter::power_strip_configuration
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    return m_configuration;
}

auto test_device::configure(const sokketter::power_strip_configuration &configuration) -> void
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    m_configuration = configuration;
}

auto test_device::try_authenticate() -> bool
{
    /**
     * @attention without an opt-in password the fake device behaves like the library base class
     * and refuses authentication; with one, the password stored in the configuration must match
     * the value the environment variable provided at construction.
     */
    if (!m_authentication_password)
    {
        return false;
    }

    return configuration().authentication.password == *m_authentication_password;
}

auto test_device::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    if (index >= m_sockets.size())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: index {} is out of range 0-{}!", this->to_string(), index, m_sockets.size());
        return std::nullopt;
    }

    return m_sockets[index];
}

auto test_device::power_socket(size_t index, bool is_toggled) -> bool
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");

    {
        const std::lock_guard<std::mutex> states_lock(gs_socket_states_mutex);

        gs_socket_states[m_serial_number][index - 1] = is_toggled;
    }

    return true;
}

auto test_device::socket_status(size_t index) -> bool
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);

    const std::lock_guard<std::mutex> states_lock(gs_socket_states_mutex);

    return gs_socket_states[m_serial_number][index - 1];
}
