#include "power_strip_factory.h"

#include <devices/energenie_eg_pms.h>
#include <devices/energenie_eg_pms2.h>
#include <devices/gembird_msis_pm.h>
#include <devices/gembird_msis_pm_2.h>
#include <devices/gembird_sis_pm.h>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>

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

auto power_strip_factory::create(std::shared_ptr<kommpot::device_communication> communication)
    -> std::shared_ptr<sokketter::power_strip>
{
    /**
     * Gembird MSIS-PM.
     */
    if (communication->information().vendor_id == gembird_msis_pm::identification().vendor_id &&
        communication->information().product_id == gembird_msis_pm::identification().product_id)
    {
        auto ptr = std::make_shared<gembird_msis_pm>();
        return ptr->initialize(communication) ? ptr : nullptr;
    }

    /**
     * Gembird MSIS-PM 2.
     */
    if (communication->information().vendor_id == gembird_msis_pm_2::identification().vendor_id &&
        communication->information().product_id == gembird_msis_pm_2::identification().product_id)
    {
        auto ptr = std::make_shared<gembird_msis_pm_2>();
        return ptr->initialize(communication) ? ptr : nullptr;
    }

    /**
     * Gembird SIS-PM.
     */
    if (communication->information().vendor_id == gembird_sis_pm::identification().vendor_id &&
        communication->information().product_id == gembird_sis_pm::identification().product_id)
    {
        auto ptr = std::make_shared<gembird_sis_pm>();
        return ptr->initialize(communication) ? ptr : nullptr;
    }

    /**
     * Energenie EG-PMS.
     */
    if (communication->information().vendor_id == energenie_eg_pms::identification().vendor_id &&
        communication->information().product_id == energenie_eg_pms::identification().product_id)
    {
        auto ptr = std::make_shared<energenie_eg_pms>();
        return ptr->initialize(communication) ? ptr : nullptr;
    }

    /**
     * Energenie EG-PMS2.
     */
    if (communication->information().vendor_id == energenie_eg_pms2::identification().vendor_id &&
        communication->information().product_id == energenie_eg_pms2::identification().product_id)
    {
        auto ptr = std::make_shared<energenie_eg_pms2>();
        return ptr->initialize(communication) ? ptr : nullptr;
    }

    SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
        "Provided communication is not supported: {}, VID{}:PID{}, at port {}!",
        communication->information().name, communication->information().vendor_id,
        communication->information().product_id, communication->information().port);

    return nullptr;
}

std::shared_ptr<sokketter::power_strip> power_strip_factory::create(
    const sokketter::power_strip_type &type)
{
    switch (type)
    {
    /**
     * Gembird MSIS-PM.
     */
    case sokketter::power_strip_type::GEMBIRD_MSIS_PM: {
        return std::make_shared<gembird_msis_pm>();
    }
    /**
     * Gembird MSIS-PM 2.
     */
    case sokketter::power_strip_type::GEMBIRD_MSIS_PM_2: {
        return std::make_shared<gembird_msis_pm_2>();
    }
    /**
     * Gembird SIS-PM.
     */
    case sokketter::power_strip_type::GEMBIRD_SIS_PM: {
        return std::make_shared<gembird_sis_pm>();
    }
    /**
     * Energenie EG-PMS.
     */
    case sokketter::power_strip_type::ENERGENIE_EG_PMS: {
        return std::make_shared<energenie_eg_pms>();
    }
    /**
     * Energenie EG-PMS2.
     */
    case sokketter::power_strip_type::ENERGENIE_EG_PMS2: {
        return std::make_shared<energenie_eg_pms2>();
    }
    default: {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "Provided power strip type is not supported: {}!", uint8_t(type));
        return nullptr;
    }
    }
}
