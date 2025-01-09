#include "power_strip_factory.h"

#include <devices/energenie_eg_base.h>

auto power_strip_factory::create(std::unique_ptr<kommpot::device_communication> communication)
    -> std::unique_ptr<sokketter::power_strip>
{
    /**
     * Gembird / Energenie EG-PMS2.
     */
    if (communication->information().vendor_id == energenie_eg_base::identification().vendor_id &&
        communication->information().product_id == energenie_eg_base::identification().product_id)
    {
        return std::make_unique<energenie_eg_base>(std::move(communication));
    }

    return nullptr;
}

auto power_strip_factory::supported_devices() -> const std::vector<kommpot::device_identification>
{
    std::vector<kommpot::device_identification> identifications;

    identifications.push_back(energenie_eg_base::identification());

    return identifications;
}
