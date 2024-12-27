#include "power_strip_base.h"

power_strip_base::power_strip_base(
    const std::unique_ptr<kommpot::device_communication> &communication)
    : m_communication(std::move(communication))
{}
