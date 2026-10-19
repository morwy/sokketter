#ifndef POWER_STRIP_FACTORY_H
#define POWER_STRIP_FACTORY_H

#pragma once

#include <libsokketter.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

namespace power_strip_factory {
    auto supported_devices(const sokketter::device_filter &filter)
        -> const std::vector<kommpot::device_identification>;

    /**
     * @brief builds targeted identifications for stored devices of network types, so they can be
     * probed at their last known addresses instead of being searched for on the whole network.
     * @param database with the stored devices.
     * @param filter stating which device types to include.
     * @return vector of kommpot device identifications with concrete addresses.
     * @attention MAC is left as a wildcard on purpose: the caller verifies the MAC of the host
     * that answers itself, so it can tell "no device at this address" apart from "address taken
     * over by another device".
     */
    auto targeted_identifications(
        const std::vector<std::shared_ptr<sokketter::power_strip>> &database,
        const sokketter::device_filter &filter) -> const std::vector<kommpot::device_identification>;

    auto create(std::shared_ptr<kommpot::device_communication> communication)
        -> std::shared_ptr<sokketter::power_strip>;
    auto create(const sokketter::power_strip_type &type) -> std::shared_ptr<sokketter::power_strip>;
}; // namespace power_strip_factory

#endif // POWER_STRIP_FACTORY_H
