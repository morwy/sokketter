#ifndef ENERGENIE_EG_PMX_X_H
#define ENERGENIE_EG_PMX_X_H

#pragma once

#include <devices/power_strip_base.h>
#include <libsokketter.h>

class energenie_eg_pmx_x : public power_strip_base
{
public:
    explicit energenie_eg_pmx_x(
        const std::unique_ptr<kommpot::device_communication> &communication);

    static auto identification() -> const kommpot::device_identification;
};

#endif // ENERGENIE_EG_PMX_X_H
