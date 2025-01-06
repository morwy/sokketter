#include "power_strip_factory.h"

#include <devices/energenie_eg_pmx_x.h>

auto power_strip_factory::create(
    const std::unique_ptr<kommpot::device_communication> &communication)
    -> std::unique_ptr<sokketter::power_strip>
{
    /**
     * Gembird / Energenie EG-PMS2.
     */
    if (communication->information().vendor_id == 0x04b4 &&
        communication->information().product_id == 0xfd15)
    {
        return std::make_unique<energenie_eg_pmx_x>(communication);
    }

    return nullptr;
}

auto power_strip_factory::supported_devices() -> const std::vector<kommpot::device_identification>
{
    std::vector<kommpot::device_identification> identifications;

    identifications.push_back(energenie_eg_pmx_x::identification());

    return identifications;
}
