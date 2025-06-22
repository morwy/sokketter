#include "energenie_eg_base.h"

#include <array>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>
#include <sstream>

/**
 * @attention interaction with device is based on the protocol described in pysispm project.
 * @link https://github.com/xypron/pysispm/blob/master/sispm/__init__.py
 */
auto energenie_eg_base::initialize(std::shared_ptr<kommpot::device_communication> communication)
    -> bool
{
    if (!power_strip_base::initialize(communication))
    {
        return false;
    }

    /**
     * Read device serial number from device since it is not available in USB descriptor.
     */
    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed opening the device communication!", this->to_string());
        return false;
    }

    kommpot::control_transfer_configuration configuration;
    configuration.request_type = 0xa1;
    configuration.request = 0x01;
    configuration.value = 0x0301;
    configuration.index = 0;

    std::array<uint8_t, 5> serial_number_raw = {0};

    if (!m_communication->read(configuration, serial_number_raw.data(), serial_number_raw.size()))
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed reading the device serial number!", this->to_string());
        return false;
    }

    m_communication->close();

    std::ostringstream serial_number;
    for (auto it = serial_number_raw.begin(); it != serial_number_raw.end(); ++it)
    {
        serial_number << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*it);
        if (it != serial_number_raw.end() - 1)
        {
            serial_number << ":";
        }
    }

    m_serial_number = serial_number.str();

    return true;
}

auto energenie_eg_base::power_socket(size_t index, bool is_toggled) -> bool
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "{}: powering socket {} {}.", this->to_string(), index,
        is_toggled ? "on" : "off");

    kommpot::control_transfer_configuration configuration;
    configuration.request_type = 0x21;
    configuration.request = 0x09;
    configuration.value = 0x0300 + 3 * index;
    configuration.index = 0;

    std::array<uint8_t, 5> buffer = {uint8_t(3 * index), 0x00, 0x00, 0x00, 0x00};
    if (is_toggled)
    {
        buffer = {uint8_t(3 * index), 0x03, 0x00, 0x00, 0x00};
    }

    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed opening the device communication!", this->to_string());
    }

    const bool is_operation_succeed =
        m_communication->write(configuration, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: failed writing the command!", this->to_string());
        return false;
    }

    return true;
}

auto energenie_eg_base::socket_status(size_t index) -> bool
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "{}: checking socket {} status.", this->to_string(), index);

    kommpot::control_transfer_configuration configuration;
    configuration.request_type = 0xa1;
    configuration.request = 0x01;
    configuration.value = 0x0300 + 3 * index;
    configuration.index = 0;

    std::array<uint8_t, 5> buffer = {uint8_t(3 * index), 0x03, 0x00, 0x00, 0x00};

    if (!m_communication->open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "{}: failed opening the device communication!", this->to_string());
    }

    const bool is_operation_succeed =
        m_communication->read(configuration, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "{}: failed reading the status!", this->to_string());
        return false;
    }

    return bool(1 & buffer[1]);
}
