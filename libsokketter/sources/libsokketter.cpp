#include "libsokketter.h"
#include "devices/power_strip_base.h"

#include <cstdint>
#include <string>
#include <utility>

#include <third-party/kommpot/libkommpot/include/libkommpot.h>

sokketter::version_information::version_information(const uint8_t major, const uint8_t minor,
    const uint8_t micro, const uint8_t nano, std::string git_hash)
    : m_major(major)
    , m_minor(minor)
    , m_micro(micro)
    , m_nano(nano)
    , m_git_hash(std::move(git_hash))
{}

auto sokketter::version_information::major() const noexcept -> uint8_t
{
    return m_major;
}

auto sokketter::version_information::minor() const noexcept -> uint8_t
{
    return m_minor;
}

auto sokketter::version_information::micro() const noexcept -> uint8_t
{
    return m_micro;
}

auto sokketter::version_information::nano() const noexcept -> uint8_t
{
    return m_nano;
}

auto sokketter::version_information::git_hash() const noexcept -> std::string
{
    return m_git_hash;
}

auto sokketter::version_information::to_string() const noexcept -> std::string
{
    return std::to_string(m_major) + "." + std::to_string(m_minor) + "." + std::to_string(m_micro) +
           "." + std::to_string(m_nano);
}

auto sokketter::version() noexcept -> sokketter::version_information
{
    return {PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_MICRO,
        PROJECT_VERSION_NANO, PROJECT_VERSION_SHA};
}

auto sokketter::power_strip::configuration() -> const power_strip_configuration &
{
    return m_configuration;
}

auto sokketter::power_strip::configure(const power_strip_configuration &configuration) -> void
{
    m_configuration = configuration;
}

auto sokketter::devices(const device_filter &filter)
    -> const std::vector<std::unique_ptr<sokketter::power_strip>>
{
    std::vector<std::unique_ptr<sokketter::power_strip>> devices;

    std::vector<kommpot::device_identification> identifications;

    kommpot::device_identification energenie_identification;
    energenie_identification.vendor_id = 0x04b4;
    energenie_identification.product_id = 0xfd15;
    identifications.push_back(energenie_identification);

    const auto kommpot_devices = kommpot::devices(identifications);
    for (const auto &communication : kommpot_devices)
    {
        auto device = std::make_unique<power_strip_base>(communication);
        if (!device)
        {
            // spdlog::error("std::make_unique() failed creating the device!");
            continue;
        }

        devices.push_back(std::move(device));
    }

    return devices;
}
