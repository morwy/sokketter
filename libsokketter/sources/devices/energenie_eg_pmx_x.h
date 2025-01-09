#ifndef ENERGENIE_EG_PMX_X_H
#define ENERGENIE_EG_PMX_X_H

#pragma once

#include <devices/power_strip_base.h>
#include <libsokketter.h>

class energenie_eg_pmx_x : public power_strip_base
{
public:
    explicit energenie_eg_pmx_x(std::unique_ptr<kommpot::device_communication> communication);

    static auto identification() -> const kommpot::device_identification;

    [[nodiscard]] auto sockets() -> const std::vector<sokketter::socket> & override;

private:
    std::vector<sokketter::socket> m_sockets;

    auto toggle_socket(size_t index, bool is_toggled) -> bool;
    auto socket_status(size_t index) -> bool;
};

#endif // ENERGENIE_EG_PMX_X_H
