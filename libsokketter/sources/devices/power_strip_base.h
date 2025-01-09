#ifndef POWER_STRIP_BASE_H
#define POWER_STRIP_BASE_H

#pragma once

#include "libsokketter.h"

#include <third-party/kommpot/libkommpot/include/libkommpot.h>

class power_strip_base : public sokketter::power_strip
{
public:
    explicit power_strip_base(std::unique_ptr<kommpot::device_communication> communication);

    [[nodiscard]] auto is_connected() const -> bool override;

protected:
    std::unique_ptr<kommpot::device_communication> m_communication;
};

#endif // POWER_STRIP_BASE_H
