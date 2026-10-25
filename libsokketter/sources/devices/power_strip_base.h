#ifndef POWER_STRIP_BASE_H
#define POWER_STRIP_BASE_H

#pragma once

#include <libsokketter.h>

#include <third-party/kommpot/libkommpot/include/libkommpot.h>

#include <functional>
#include <mutex>
#include <optional>

class power_strip_base : public sokketter::power_strip
{
public:
    power_strip_base() = default;

    virtual bool initialize(std::shared_ptr<kommpot::device_communication> communication);

    bool copyFrom(const sokketter::power_strip &other);

    [[nodiscard]] auto socket(const size_t &index)
        -> const std::optional<std::reference_wrapper<sokketter::socket>> override;

    [[nodiscard]] auto is_connected() const -> bool override;

    [[nodiscard]] auto configuration() const -> sokketter::power_strip_configuration override;

    auto configure(const sokketter::power_strip_configuration &configuration) -> void override;

protected:
    std::shared_ptr<kommpot::device_communication> m_communication = nullptr;

    /**
     * @brief serializes the mutable device state, which the enumeration thread mutates while the
     * GUI and worker threads read it.
     * @attention guarded members: the communication (@ref m_communication), the configuration
     * (@ref m_configuration), and the serial number (@ref energenie_eg_base::m_serial_number,
     * written by the derived device classes).
     * @attention lock order: the device-specific communication mutexes are taken before this
     * mutex, never the other way around.
     */
    mutable std::mutex m_state_mutex;

    /**
     * @brief applies an update to the stored configuration under the state mutex, so that
     * partial configuration writes done during initialization cannot race with configuration
     * reads/copies on other threads.
     * @param update callable mutating the live configuration.
     */
    auto update_configuration(
        const std::function<void(sokketter::power_strip_configuration &)> &update) -> void;
};

#endif // POWER_STRIP_BASE_H
