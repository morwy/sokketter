#ifndef GEMBIRD_MSIS_PM_H
#define GEMBIRD_MSIS_PM_H

#pragma once

#include <devices/energenie_eg_base.h>

class gembird_msis_pm : public energenie_eg_base
{
public:
    gembird_msis_pm();
    ~gembird_msis_pm();

    auto initialize(std::shared_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::device_identification;
};

#endif // GEMBIRD_MSIS_PM_H
