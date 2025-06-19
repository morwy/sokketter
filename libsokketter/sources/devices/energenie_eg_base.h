#ifndef ENERGENIE_EG_BASE_H
#define ENERGENIE_EG_BASE_H

#pragma once

#include <devices/power_strip_base.h>
#include <libsokketter.h>

class energenie_eg_base : public power_strip_base
{
public:
    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

protected:
    std::string m_serial_number = "";
    size_t m_socket_number = 0;

    auto power_socket(size_t index, bool is_toggled) -> bool;
    auto socket_status(size_t index) -> bool;
};

#endif // ENERGENIE_EG_BASE_H
