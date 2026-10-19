#ifndef POWER_STRIP_FACTORY_H
#define POWER_STRIP_FACTORY_H

#pragma once

#include <libsokketter.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

namespace power_strip_factory {
    auto supported_devices(const sokketter::device_filter &filter)
        -> const std::vector<kommpot::device_identification>;

    /**
     * @brief a targeted probe of a single stored network device: the identification to probe with
     * and the stored device the probe was built for.
     */
    struct targeted_probe
    {
        kommpot::http_device_identification identification;
        std::shared_ptr<sokketter::power_strip> device;
    };

    /**
     * @brief builds targeted probes for stored devices of network types, so they can be probed at
     * their last known addresses instead of being searched for on the whole network.
     * @param database with the stored devices.
     * @param filter stating which device types to include.
     * @return vector of targeted probes carrying the identification to probe and its stored device.
     * @attention MAC is left as a wildcard on purpose: the caller verifies the MAC of the host
     * that answers itself, so it can tell "no device at this address" apart from "address taken
     * over by another device".
     */
    auto targeted_probes(
        const std::vector<std::shared_ptr<sokketter::power_strip>> &database,
        const sokketter::device_filter &filter) -> const std::vector<targeted_probe>;

    auto create(std::shared_ptr<kommpot::device_communication> communication)
        -> std::shared_ptr<sokketter::power_strip>;
    auto create(const sokketter::power_strip_type &type) -> std::shared_ptr<sokketter::power_strip>;
}; // namespace power_strip_factory

#endif // POWER_STRIP_FACTORY_H
