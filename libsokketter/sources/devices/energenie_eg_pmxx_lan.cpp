#include "energenie_eg_pmxx_lan.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

/**
 * @attention
 * @link
 */
energenie_eg_pmxx_lan::energenie_eg_pmxx_lan()
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: constructed object {}.", __FUNCTION__, static_cast<void *>(this));

    sokketter::power_strip_configuration configuration;
    configuration.type = sokketter::power_strip_type::ENERGENIE_EG_PMXX_LAN;
    this->configure(configuration);

    /**
     * Configure sockets.
     */
    m_socket_number = 4;

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(&energenie_eg_pmxx_lan::power_socket, this, std::placeholders::_1,
                std::placeholders::_2),
            std::bind(&energenie_eg_pmxx_lan::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

energenie_eg_pmxx_lan::~energenie_eg_pmxx_lan()
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destructed object {}.", this->to_string(),
        static_cast<void *>(this));
}

auto energenie_eg_pmxx_lan::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    const auto &identification_variant = communication->identification();
    const auto *identification =
        std::get_if<kommpot::ethernet_device_identification>(&identification_variant);
    if (identification == nullptr)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Provided identification is not Ethernet.");
        return false;
    }

    /**
     * @attention swap communication to HTTP after discovering required host.
     */
    if (communication->type() == kommpot::communication_type::ETHERNET)
    {
        const kommpot::http_device_identification http_identification{identification->ip, 80};
        auto http_communication = kommpot::device(http_identification);
        if (!power_strip_base::initialize(http_communication))
        {
            return false;
        }
    }
    else
    {
        if (!power_strip_base::initialize(communication))
        {
            return false;
        }
    }

    auto configuration = this->configuration();

    m_serial_number = identification->mac;
    configuration.id = identification->mac;
    configuration.address = identification->ip;

    this->configure(configuration);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: initialization.", this->to_string());

    return true;
}

auto energenie_eg_pmxx_lan::identification() -> const kommpot::ethernet_device_identification
{
    kommpot::ethernet_device_identification identification;

    identification.ip = "*";
    identification.port = 5000;
    identification.mac = "88:B6:27:*";
    identification.protocol = kommpot::ethernet_protocol_type::TCP;

    return identification;
}

auto energenie_eg_pmxx_lan::connect_if_not_yet() -> bool
{
    if (!m_communication->is_open())
    {
        if (!m_communication->open())
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: failed to open communication.", this->to_string());
            return false;
        }
    }

    return true;
}

auto energenie_eg_pmxx_lan::disconnect() -> void
{
    if (m_communication->is_open())
    {
        m_communication->close();
    }
}

auto energenie_eg_pmxx_lan::login(const std::string &password) -> bool
{
    connect_if_not_yet();

    auto transfer = kommpot::http_transfer_configuration();
    transfer.type = kommpot::http_transfer_type::POST;
    transfer.resource_path = "/login.html";
    transfer.body = "pw=" + password;
    transfer.content_type = "application/x-www-form-urlencoded";

    std::string body = "pw=" + password;

    return m_communication->write(transfer, body.data(), body.size());
}

auto energenie_eg_pmxx_lan::logout() -> bool
{
    const auto transfer =
        kommpot::http_transfer_configuration{kommpot::http_transfer_type::GET, "/login.html"};
    const bool result = m_communication->read(transfer, nullptr, 0);

    disconnect();

    return result;
}

auto energenie_eg_pmxx_lan::power_socket(size_t index, bool is_toggled) -> bool
{
    if (m_communication == nullptr)
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "{}: skipping powering socket due to disconnected status.", this->to_string(), index);
        return false;
    }

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");

    if (!login(m_password))
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: failed to login to device.", this->to_string());
        return false;
    }

    const auto transfer =
        kommpot::http_transfer_configuration{kommpot::http_transfer_type::POST, "/"};
    std::string body = "cte" + std::to_string(index) + "=" + std::string(is_toggled ? "1" : "0");

    const bool result = m_communication->write(transfer, body.data(), body.size());

    if (!logout())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed to logout from device.", this->to_string());
    }

    return result;
}

auto energenie_eg_pmxx_lan::socket_status(size_t index) -> bool
{
    if (m_communication == nullptr)
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "{}: skipping checking socket status due to disconnected status.", this->to_string(),
            index);
        return false;
    }

    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);

    return false;
}
