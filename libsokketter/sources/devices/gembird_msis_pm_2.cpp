#include "gembird_msis_pm_2.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

gembird_msis_pm_2::gembird_msis_pm_2()
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: constructed object {}.", __FUNCTION__, static_cast<void *>(this));

    m_configuration.type = sokketter::power_strip_type::GEMBIRD_MSIS_PM_2;

    /**
     * Configure sockets.
     */
    m_socket_number = 1;

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(&gembird_msis_pm_2::power_socket, this, std::placeholders::_1,
                std::placeholders::_2),
            std::bind(&gembird_msis_pm_2::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

gembird_msis_pm_2::~gembird_msis_pm_2()
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destructed object {}.", this->to_string(),
        static_cast<void *>(this));
}

auto gembird_msis_pm_2::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    if (!energenie_eg_base::initialize(communication))
    {
        return false;
    }

    const auto &identification_variant = m_communication->identification();
    const auto *identification =
        std::get_if<kommpot::usb_device_identification>(&identification_variant);
    if (identification == nullptr)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Provided identification is not USB.");
        return false;
    }

    m_configuration.id = m_serial_number;
    m_configuration.address = std::string("USB:") + identification->port;

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: initialization.", this->to_string());

    return true;
}

auto gembird_msis_pm_2::identification() -> const kommpot::usb_device_identification
{
    kommpot::usb_device_identification identitication;

    identitication.vendor_id = 0x04b4;
    identitication.product_id = 0xfd12;

    return identitication;
}
