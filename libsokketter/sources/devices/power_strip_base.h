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

    virtual bool initialize(std::unique_ptr<kommpot::device_communication> communication);

    [[nodiscard]] auto sockets() const -> const std::vector<sokketter::socket> & override;

    [[nodiscard]] auto socket(const size_t &index)
        -> const std::optional<std::reference_wrapper<sokketter::socket>> override;

    [[nodiscard]] auto is_connected() const -> bool override;

    [[nodiscard]] auto extractCommunication() -> std::unique_ptr<kommpot::device_communication>;

protected:
    std::unique_ptr<kommpot::device_communication> m_communication = nullptr;
    std::vector<sokketter::socket> m_sockets;
};

#endif // POWER_STRIP_BASE_H
