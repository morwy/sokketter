#include "energenie_eg_base.h"

#include <array>
#include <spdlog/spdlog.h>
#include <sstream>

/**
 * @attention interaction with device is based on the protocol described in pysispm project.
 * @link https://github.com/xypron/pysispm/blob/master/sispm/__init__.py
 */
energenie_eg_base::energenie_eg_base(std::unique_ptr<kommpot::device_communication> communication)
    : power_strip_base(std::move(communication))
{
    /**
     * Read device serial number from device since it is not available in USB descriptor.
     */
    if (!m_communication->open())
    {
        SPDLOG_ERROR("{}: failed opening the device communication!", this->to_string());
    }

    kommpot::control_transfer_configuration configuration;
    configuration.request_type = 0xa1;
    configuration.request = 0x01;
    configuration.value = 0x0301;
    configuration.index = 0;

    std::array<uint8_t, 5> serial_number_raw = {0};

    if (!m_communication->read(configuration, serial_number_raw.data(), serial_number_raw.size()))
    {
        SPDLOG_ERROR("{}: failed reading the device serial number!", this->to_string());
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
}

auto energenie_eg_base::sockets() -> const std::vector<sokketter::socket> &
{
    return m_sockets;
}

auto energenie_eg_base::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    if (index >= m_sockets.size())
    {
        SPDLOG_ERROR(
            "{}: index {} is out of range 0-{}!", this->to_string(), index, m_sockets.size());
        return std::nullopt;
    }

    return m_sockets[index];
}

auto energenie_eg_base::power_socket(size_t index, bool is_toggled) -> bool
{
    SPDLOG_DEBUG("{}: powering socket {} {}.", this->to_string(), index, is_toggled ? "on" : "off");

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
        SPDLOG_ERROR("{}: failed opening the device communication!", this->to_string());
    }

    const bool is_operation_succeed =
        m_communication->write(configuration, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        SPDLOG_ERROR("{}: failed writing the command!", this->to_string());
        return false;
    }

    return true;
}

auto energenie_eg_base::socket_status(size_t index) -> bool
{
    SPDLOG_DEBUG("{}: checking socket {} status.", this->to_string(), index);

    kommpot::control_transfer_configuration configuration;
    configuration.request_type = 0xa1;
    configuration.request = 0x01;
    configuration.value = 0x0300 + 3 * index;
    configuration.index = 0;

    std::array<uint8_t, 5> buffer = {uint8_t(3 * index), 0x03, 0x00, 0x00, 0x00};

    if (!m_communication->open())
    {
        SPDLOG_ERROR("{}: failed opening the device communication!", this->to_string());
    }

    const bool is_operation_succeed =
        m_communication->read(configuration, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        SPDLOG_ERROR("{}: failed reading the status!", this->to_string());
        return false;
    }

    return bool(1 & buffer[1]);
}
