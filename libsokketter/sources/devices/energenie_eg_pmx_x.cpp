#include "energenie_eg_pmx_x.h"

#include <sstream>

energenie_eg_pmx_x::energenie_eg_pmx_x(
    const std::unique_ptr<kommpot::device_communication> &communication)
    : power_strip_base(communication)
{
    /**
     * Read device serial number before applying the configuration.
     */
    communication->open();

    kommpot::endpoint_information endpoint_information;
    endpoint_information.parameters.set("libusb_transfer_type", std::string("control"));
    endpoint_information.parameters.set("libusb_ctrl_type", 0xa1);
    endpoint_information.parameters.set("libusb_ctrl_request", 0x01);
    endpoint_information.parameters.set("libusb_ctrl_value", 0x0301);
    endpoint_information.parameters.set("libusb_ctrl_index", 0);

    std::array<uint8_t, 5> serial_number_raw = {0};

    if (!communication->read(
            endpoint_information, serial_number_raw.data(), serial_number_raw.size()))
    {}

    communication->close();

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
    configuration.address = std::string("USB:") + communication->information().port;

    this->configure(configuration);
}

const kommpot::device_identification energenie_eg_pmx_x::identification()
{
    kommpot::device_identification identitication;

    identitication.vendor_id = 0x04b4;
    identitication.product_id = 0xfd15;

    return identitication;
}
