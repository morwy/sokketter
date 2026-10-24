#include "power_strip_base.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

bool power_strip_base::initialize(std::shared_ptr<kommpot::device_communication> communication)
{
    if (communication == nullptr)
    {
        return false;
    }

    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    m_communication = communication;

    return true;
}

bool power_strip_base::copyFrom(const power_strip &other)
{
    /**
     * @attention backwards compatibility: if the authentication type is unknown, it means that
     * the power strip was saved before the authentication type was introduced, so we need to
     * set it to the default value.
     */
    auto other_configuration = other.configuration();

    if (other_configuration.authentication.type ==
        sokketter::power_strip_authentication_type::UNKNOWN)
    {
        other_configuration.authentication.type = this->configuration().authentication.type;
    }

    configure(other_configuration);

    const auto &other_sockets = other.sockets();

    const size_t min_number_of_sockets = std::min(m_sockets.size(), other_sockets.size());
    for (size_t index = 0; index < min_number_of_sockets; index++)
    {
        m_sockets[index].configure(other_sockets[index].configuration());
    }

    return true;
}

auto power_strip_base::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    if (index >= m_sockets.size())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: index {} is out of range 0-{}!",
            this->to_string(), index, m_sockets.size());
        return std::nullopt;
    }

    return m_sockets[index];
}

auto power_strip_base::is_connected() const -> bool
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    return m_communication != nullptr;
}

auto power_strip_base::configuration() const -> sokketter::power_strip_configuration
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    return m_configuration;
}

auto power_strip_base::configure(const sokketter::power_strip_configuration &configuration) -> void
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    m_configuration = configuration;
}

auto power_strip_base::update_configuration(
    const std::function<void(sokketter::power_strip_configuration &)> &update) -> void
{
    const std::lock_guard<std::mutex> state_lock(m_state_mutex);

    update(m_configuration);
}
