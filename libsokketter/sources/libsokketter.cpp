#include "libsokketter.h"

#include <cstdint>
#include <string>
#include <utility>

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

const std::vector<sokketter::power_strip> sokketter::devices(const device_filter &filter)
{
    std::vector<sokketter::power_strip> devices;

    return devices;
}
