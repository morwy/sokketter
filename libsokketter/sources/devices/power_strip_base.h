#ifndef POWER_STRIP_BASE_H
#define POWER_STRIP_BASE_H

#pragma once

#include <libsokketter.h>

#include <third-party/kommpot/libkommpot/include/libkommpot.h>

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

protected:
    std::shared_ptr<kommpot::device_communication> m_communication = nullptr;
};

#endif // POWER_STRIP_BASE_H
