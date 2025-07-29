#include "gembird_sis_pm.h"

#include <sokketter_core.h>
#include <spdlog/spdlog.h>

gembird_sis_pm::gembird_sis_pm()
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: constructed object {}.", __FUNCTION__, static_cast<void *>(this));

    sokketter::power_strip_configuration configuration;
    configuration.type = sokketter::power_strip_type::GEMBIRD_SIS_PM;
    this->configure(configuration);

    /**
     * Configure sockets.
     */
    m_socket_number = 4;

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(
                &gembird_sis_pm::power_socket, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&gembird_sis_pm::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

gembird_sis_pm::~gembird_sis_pm()
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: destructed object {}.", this->to_string(),
        static_cast<void *>(this));
}

auto gembird_sis_pm::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    if (!energenie_eg_base::initialize(communication))
    {
        return false;
    }

    auto configuration = this->configuration();

    configuration.id = m_serial_number;
    configuration.address = std::string("USB:") + m_communication->information().port;

    this->configure(configuration);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: initialization.", this->to_string());

    return true;
}

auto gembird_sis_pm::identification() -> const kommpot::usb_device_identification
{
    kommpot::usb_device_identification identitication;

    identitication.vendor_id = 0x04b4;
    identitication.product_id = 0xfd11;

    return identitication;
}
