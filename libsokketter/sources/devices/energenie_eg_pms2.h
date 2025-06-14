#ifndef ENERGENIE_EG_PMS2_H
#define ENERGENIE_EG_PMS2_H

#pragma once

#include <devices/energenie_eg_base.h>

class energenie_eg_pms2 : public energenie_eg_base
{
public:
    energenie_eg_pms2();
    ~energenie_eg_pms2();

    auto initialize(std::unique_ptr<kommpot::device_communication> communication) -> bool override;

    static auto identification() -> const kommpot::device_identification;
};

#endif // ENERGENIE_EG_PMS2_H
