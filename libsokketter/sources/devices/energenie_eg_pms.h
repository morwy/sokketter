#ifndef ENERGENIE_EG_PMS_H
#define ENERGENIE_EG_PMS_H

#pragma once

#include <devices/energenie_eg_base.h>

class energenie_eg_pms : public energenie_eg_base
{
public:
    energenie_eg_pms();
    ~energenie_eg_pms();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::usb_device_identification;
};

#endif // ENERGENIE_EG_PMS_H
