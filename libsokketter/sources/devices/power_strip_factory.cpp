#include "power_strip_factory.h"

#include <devices/energenie_eg_pms.h>
#include <devices/energenie_eg_pms2.h>
#include <devices/gembird_msis_pm.h>
#include <devices/gembird_msis_pm_2.h>
#include <devices/gembird_sis_pm.h>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>

auto power_strip_factory::create(std::unique_ptr<kommpot::device_communication> communication)
    -> std::unique_ptr<sokketter::power_strip>
{
    /**
     * Gembird MSIS-PM.
     */
    if (communication->information().vendor_id == gembird_msis_pm::identification().vendor_id &&
        communication->information().product_id == gembird_msis_pm::identification().product_id)
    {
        return std::make_unique<gembird_msis_pm>(std::move(communication));
    }

    /**
     * Gembird MSIS-PM 2.
     */
    if (communication->information().vendor_id == gembird_msis_pm_2::identification().vendor_id &&
        communication->information().product_id == gembird_msis_pm_2::identification().product_id)
    {
        return std::make_unique<gembird_msis_pm_2>(std::move(communication));
    }

    /**
     * Gembird SIS-PM.
     */
    if (communication->information().vendor_id == gembird_sis_pm::identification().vendor_id &&
        communication->information().product_id == gembird_sis_pm::identification().product_id)
    {
        return std::make_unique<gembird_sis_pm>(std::move(communication));
    }

    /**
     * Energenie EG-PMS.
     */
    if (communication->information().vendor_id == energenie_eg_pms::identification().vendor_id &&
        communication->information().product_id == energenie_eg_pms::identification().product_id)
    {
        return std::make_unique<energenie_eg_pms>(std::move(communication));
    }

    /**
     * Energenie EG-PMS2.
     */
    if (communication->information().vendor_id == energenie_eg_pms2::identification().vendor_id &&
        communication->information().product_id == energenie_eg_pms2::identification().product_id)
    {
        return std::make_unique<energenie_eg_pms2>(std::move(communication));
    }

    SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
        "Provided communication is not supported: {}, VID{}:PID{}, at port {}!",
        communication->information().name, communication->information().vendor_id,
        communication->information().product_id, communication->information().port);

    return nullptr;
}

auto power_strip_factory::supported_devices() -> const std::vector<kommpot::device_identification>
{
    std::vector<kommpot::device_identification> identifications;

    identifications.push_back(gembird_msis_pm::identification());
    identifications.push_back(gembird_msis_pm_2::identification());
    identifications.push_back(gembird_sis_pm::identification());
    identifications.push_back(energenie_eg_pms::identification());
    identifications.push_back(energenie_eg_pms2::identification());

    return identifications;
}
