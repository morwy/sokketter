#include "power_strip_factory.h"

#include <devices/energenie_eg_pmx_x.h>

auto power_strip_factory::create(
    const std::unique_ptr<kommpot::device_communication> &communication)
    -> std::unique_ptr<sokketter::power_strip>
{
    /**
     * Gembird / Energenie EG-PMS2.
     */
    if (communication->information().vendor_id == energenie_eg_pmx_x::identification().vendor_id &&
        communication->information().product_id == energenie_eg_pmx_x::identification().product_id)
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
