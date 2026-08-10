#ifndef GEMBIRD_SIS_PM_H
#define GEMBIRD_SIS_PM_H

#pragma once

#include <devices/energenie_eg_base.h>

class gembird_sis_pm : public energenie_eg_base
{
public:
    gembird_sis_pm();
    ~gembird_sis_pm();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::usb_device_identification;
};

#endif // GEMBIRD_SIS_PM_H
