#include "energenie_eg_pms2.h"

energenie_eg_pms2::energenie_eg_pms2(std::unique_ptr<kommpot::device_communication> communication)
    : energenie_eg_base(std::move(communication))
{
    sokketter::power_strip_configuration configuration;
    configuration.name = "Unnamed power strip";
    configuration.description = "";
    configuration.type = sokketter::power_strip_type::ENERGENIE_EG_PMS2;
    configuration.id = m_serial_number;
    configuration.address = std::string("USB:") + m_communication->information().port;

    this->configure(configuration);

    /**
     * Configure sockets.
     */
    m_socket_number = 4;

    for (size_t socket_index = 1; socket_index < m_socket_number + 1; socket_index++)
    {
        sokketter::socket socket(socket_index,
            std::bind(&energenie_eg_pms2::power_socket, this, std::placeholders::_1,
                std::placeholders::_2),
            std::bind(&energenie_eg_pms2::socket_status, this, std::placeholders::_1));
        m_sockets.push_back(socket);
    }
}

auto energenie_eg_pms2::identification() -> const kommpot::device_identification
{
    kommpot::device_identification identitication;

    identitication.vendor_id = 0x04b4;
    identitication.product_id = 0xfd15;

    return identitication;
}
