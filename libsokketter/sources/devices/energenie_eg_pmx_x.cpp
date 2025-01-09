#include "energenie_eg_pmx_x.h"

#include <sstream>

/**
 * @attention interaction with device is based on the protocol described in pysispm project.
 * @link https://github.com/xypron/pysispm/blob/master/sispm/__init__.py
 */
energenie_eg_pmx_x::energenie_eg_pmx_x(std::unique_ptr<kommpot::device_communication> communication)
    : power_strip_base(std::move(communication))
{
    /**
     * Read device serial number from device since it is not available in USB descriptor.
     */
    m_communication->open();

    kommpot::endpoint_information endpoint_information;
    endpoint_information.parameters.set<std::string>("libusb_transfer_type", "control");
    endpoint_information.parameters.set<int>("libusb_ctrl_type", 0xa1);
    endpoint_information.parameters.set<int>("libusb_ctrl_request", 0x01);
    endpoint_information.parameters.set<int>("libusb_ctrl_value", 0x0301);
    endpoint_information.parameters.set<int>("libusb_ctrl_index", 0);

    std::array<uint8_t, 5> serial_number_raw = {0};

    if (!m_communication->read(
            endpoint_information, serial_number_raw.data(), serial_number_raw.size()))
    {}

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

    sokketter::power_strip_configuration configuration;
    configuration.name = "Unnamed power strip";
    configuration.description = "";
    configuration.type = sokketter::power_strip_type::ENERGENIE_PMx_x;
    configuration.id = serial_number.str();
    configuration.address = std::string("USB:") + m_communication->information().port;

    this->configure(configuration);

    /**
     * Configure sockets.
     */
    for (size_t socket_index = 1; socket_index < 5; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(&energenie_eg_pmx_x::toggle_socket, this, std::placeholders::_1,
                std::placeholders::_2),
            std::bind(&energenie_eg_pmx_x::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

auto energenie_eg_pmx_x::identification() -> const kommpot::device_identification
{
    kommpot::device_identification identitication;

    identitication.vendor_id = 0x04b4;
    identitication.product_id = 0xfd15;

    return identitication;
}

auto energenie_eg_pmx_x::sockets() -> const std::vector<sokketter::socket> &
{
    return m_sockets;
}

auto energenie_eg_pmx_x::toggle_socket(size_t index, bool is_toggled) -> bool
{
    kommpot::endpoint_information endpoint;
    endpoint.parameters.set<std::string>("libusb_transfer_type", "control");
    endpoint.parameters.set<int>("libusb_ctrl_type", 0x21);
    endpoint.parameters.set<int>("libusb_ctrl_request", 0x09);
    endpoint.parameters.set<int>("libusb_ctrl_value", 0x0300 + 3 * index);
    endpoint.parameters.set<int>("libusb_ctrl_index", 0);

    std::array<uint8_t, 5> buffer = {uint8_t(3 * index), 0x00, 0x00, 0x00, 0x00};
    if (is_toggled)
    {
        buffer = {uint8_t(3 * index), 0x03, 0x00, 0x00, 0x00};
    }

    m_communication->open();

    bool is_operation_succeed = m_communication->write(endpoint, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        return false;
    }

    return true;
}

auto energenie_eg_pmx_x::socket_status(size_t index) -> bool
{
    kommpot::endpoint_information endpoint;
    endpoint.parameters.set<std::string>("libusb_transfer_type", "control");
    endpoint.parameters.set<int>("libusb_ctrl_type", 0xa1);
    endpoint.parameters.set<int>("libusb_ctrl_request", 0x01);
    endpoint.parameters.set<int>("libusb_ctrl_value", 0x0300 + 3 * index);
    endpoint.parameters.set<int>("libusb_ctrl_index", 0);

    std::array<uint8_t, 5> buffer = {uint8_t(3 * index), 0x03, 0x00, 0x00, 0x00};

    m_communication->open();

    bool is_operation_succeed = m_communication->read(endpoint, buffer.data(), buffer.size());

    m_communication->close();

    if (!is_operation_succeed)
    {
        return false;
    }

    return bool(1 & buffer[1]);
}
