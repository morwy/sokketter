#ifndef GEMBIRD_SIS_PM_H
#define GEMBIRD_SIS_PM_H

#pragma once

#include <devices/energenie_eg_base.h>

class gembird_sis_pm : public energenie_eg_base
{
public:
    explicit gembird_sis_pm(std::unique_ptr<kommpot::device_communication> communication);
    ~gembird_sis_pm();

    static auto identification() -> const kommpot::device_identification;
};

#endif // GEMBIRD_SIS_PM_H
