#ifndef ENERGENIE_EG_PMXX_LAN_H
#define ENERGENIE_EG_PMXX_LAN_H

#pragma once

#include <devices/energenie_eg_base.h>

class energenie_eg_pmxx_lan : public energenie_eg_base
{
public:
    energenie_eg_pmxx_lan();
    ~energenie_eg_pmxx_lan();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::ethernet_device_identification;

private:
    auto power_socket(size_t index, bool is_toggled) -> bool override;
    auto socket_status(size_t index) -> bool override;
};

#endif // ENERGENIE_EG_PMXX_LAN_H
