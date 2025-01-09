#ifndef ENERGENIE_EG_PMS_H
#define ENERGENIE_EG_PMS_H

#pragma once

#include <devices/energenie_eg_base.h>

class energenie_eg_pms : public energenie_eg_base
{
public:
    explicit energenie_eg_pms(std::unique_ptr<kommpot::device_communication> communication);

    static auto identification() -> const kommpot::device_identification;
};

#endif // ENERGENIE_EG_PMS_H
