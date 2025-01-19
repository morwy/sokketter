#ifndef GEMBIRD_MSIS_PM_H
#define GEMBIRD_MSIS_PM_H

#pragma once

#include <devices/energenie_eg_base.h>

class gembird_msis_pm : public energenie_eg_base
{
public:
    explicit gembird_msis_pm(std::unique_ptr<kommpot::device_communication> communication);

    static auto identification() -> const kommpot::device_identification;
};

#endif // GEMBIRD_MSIS_PM_H
