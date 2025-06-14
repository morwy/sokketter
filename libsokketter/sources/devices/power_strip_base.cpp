#include "power_strip_base.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

bool power_strip_base::initialize(std::unique_ptr<kommpot::device_communication> communication)
{
    if (communication == nullptr)
    {
        return false;
    }

    m_communication = std::move(communication);

    return true;
}

auto power_strip_base::sockets() const -> const std::vector<sokketter::socket> &
{
    return m_sockets;
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
    return m_communication != nullptr;
}

auto power_strip_base::extractCommunication() -> std::unique_ptr<kommpot::device_communication>
{
    return std::move(m_communication);
}
