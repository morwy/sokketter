#ifndef POWER_STRIP_BASE_H
#define POWER_STRIP_BASE_H

#pragma once

#include "libsokketter.h"

#include <third-party/kommpot/libkommpot/include/libkommpot.h>

class power_strip_base : public sokketter::power_strip
{
public:
    explicit power_strip_base(const std::unique_ptr<kommpot::device_communication> &communication);

private:
    const std::unique_ptr<kommpot::device_communication> &m_communication;
};

#endif // POWER_STRIP_BASE_H
