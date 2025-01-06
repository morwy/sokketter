#ifndef POWER_STRIP_FACTORY_H
#define POWER_STRIP_FACTORY_H

#pragma once

#include <libsokketter.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

namespace power_strip_factory {
    auto supported_devices() -> const std::vector<kommpot::device_identification>;
    auto create(const std::unique_ptr<kommpot::device_communication> &communication)
        -> std::unique_ptr<sokketter::power_strip>;
}; // namespace power_strip_factory

#endif // POWER_STRIP_FACTORY_H
