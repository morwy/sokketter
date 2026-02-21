#include "energenie_eg_pmxx_lan.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

/**
 * @attention interaction with device is based on the protocol described in egctl project.
 * @link https://github.com/unterwulf/egctl/blob/master/egctl.c
 */
constexpr uint16_t MAX_PASSWORD_BYTES = 8;

struct eg_lan_password_struct
{
    std::array<uint8_t, MAX_PASSWORD_BYTES> value;

    eg_lan_password_struct()
    {
        value.fill(0x20);
    }
} __attribute__((packed));

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
    if (!power_strip_base::initialize(communication))
    {
        return false;
    }

    const auto &identification_variant = m_communication->identification();
    const auto *identification =
        std::get_if<kommpot::ethernet_device_identification>(&identification_variant);
    if (identification == nullptr)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Provided identification is not Ethernet.");
        return false;
    }

    std::string mac_lower = identification->mac;
    std::transform(mac_lower.begin(), mac_lower.end(), mac_lower.begin(), ::tolower);

    if (mac_lower.find("88:b6:27") == std::string::npos && mac_lower.find("88:b6:27") != 0)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Not an Energenie device, skipping.");
        return false;
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

    return false;
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
