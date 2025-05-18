#ifndef ENERGENIE_EG_BASE_H
#define ENERGENIE_EG_BASE_H

#pragma once

#include <devices/power_strip_base.h>
#include <libsokketter.h>

#include <optional>

class energenie_eg_base : public power_strip_base
{
public:
    explicit energenie_eg_base(std::unique_ptr<kommpot::device_communication> communication);

    [[nodiscard]] auto sockets() -> const std::vector<sokketter::socket> & override;

    [[nodiscard]] auto socket(const size_t &index)
        -> const std::optional<std::reference_wrapper<sokketter::socket>> override;

protected:
    std::vector<sokketter::socket> m_sockets;
    std::string m_serial_number = "";
    size_t m_socket_number = 0;

    auto power_socket(size_t index, bool is_toggled) -> bool;
    auto socket_status(size_t index) -> bool;
};

#endif // ENERGENIE_EG_BASE_H
