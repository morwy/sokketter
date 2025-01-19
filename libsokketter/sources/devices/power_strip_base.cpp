#include "power_strip_base.h"

power_strip_base::power_strip_base(std::unique_ptr<kommpot::device_communication> communication)
    : m_communication(std::move(communication))
{}

auto power_strip_base::is_connected() const -> bool
{
    return m_communication != nullptr;
}
