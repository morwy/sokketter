#include "libsokketter.h"

#include <cstdint>
#include <string>
#include <utility>

#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <spdlog/spdlog.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

bool is_internal_testing_enabled()
{
    const std::string &name = "LIBSOKKETTER_TESTING_ENABLED";
    const char *value = std::getenv(name.c_str());
    if (value == nullptr)
    {
        return false;
    }

    return std::string(value) == "1";
}

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

sokketter::socket::socket(const size_t index, std::function<bool(size_t, bool)> power_cb,
    std::function<bool(size_t)> status_cb)
    : m_index(index)
    , m_power_cb(power_cb)
    , m_status_cb(status_cb)
{}

auto sokketter::socket::configuration() const noexcept -> const socket_configuration &
{
    return m_configuration;
}

auto sokketter::socket::configure(const socket_configuration &configuration) -> void
{
    m_configuration = configuration;
}

bool sokketter::socket::power(const bool &on) const noexcept
{
    if (m_power_cb == nullptr)
    {
        return false;
    }

    return m_power_cb(m_index, on);
}

auto sokketter::socket::is_powered_on() const noexcept -> bool
{
    if (m_status_cb == nullptr)
    {
        return false;
    }

    return m_status_cb(m_index);
}

auto sokketter::socket::to_string() const noexcept -> std::string
{
    return this->configuration().name + std::string(", status: ") +
           std::string(is_powered_on() ? "on" : "off");
}

std::string sokketter::power_strip_type_to_string(const power_strip_type &type)
{
    switch (type)
    {
    case power_strip_type::TEST_DEVICE: {
        return "TEST DEVICE";
    }
    case power_strip_type::GEMBIRD_MSIS_PM: {
        return "Gembird MSIS-PM";
    }
    case power_strip_type::GEMBIRD_SIS_PM: {
        return "Gembird SIS-PM";
    }
    case power_strip_type::GEMBIRD_MSIS_PM_2: {
        return "Gembird MSIS-PM (2)";
    }
    case power_strip_type::ENERGENIE_EG_PMS: {
        return "Energenie EG-PMS";
    }
    case power_strip_type::ENERGENIE_EG_PMS2: {
        return "Energenie EG-PMS2";
    }
    default: {
        return "Unknown";
    }
    }
}

auto sokketter::power_strip::configuration() const noexcept -> const power_strip_configuration &
{
    return m_configuration;
}

auto sokketter::power_strip::configure(const power_strip_configuration &configuration) -> void
{
    m_configuration = configuration;
}

auto sokketter::power_strip::to_string() const noexcept -> std::string
{
    return this->configuration().name + " (" +
           power_strip_type_to_string(this->configuration().type) + ", " +
           this->configuration().id + ", located at " + this->configuration().address + ")";
}

auto sokketter::devices(const device_filter &filter)
    -> const std::vector<std::unique_ptr<sokketter::power_strip>>
{
    auto logger = spdlog::get("libkommpot");
    if (logger != nullptr)
    {
        logger->set_level(spdlog::level::err);
    }

    std::vector<std::unique_ptr<sokketter::power_strip>> devices;

    if (is_internal_testing_enabled())
    {
        devices.push_back(std::make_unique<test_device>());
        return devices;
    }

    const auto supported_devices = power_strip_factory::supported_devices();
    auto communications = kommpot::devices(supported_devices);
    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(std::move(communication));
        if (!device)
        {
            // spdlog::error("std::make_unique() failed creating the device!");
            continue;
        }

        devices.push_back(std::move(device));
    }

    return devices;
}

auto sokketter::device(const size_t &index) -> const std::unique_ptr<sokketter::power_strip>
{
    auto logger = spdlog::get("libkommpot");
    if (logger != nullptr)
    {
        logger->set_level(spdlog::level::err);
    }

    if (is_internal_testing_enabled())
    {
        return std::make_unique<test_device>();
    }

    const auto supported_devices = power_strip_factory::supported_devices();

    auto communications = kommpot::devices(supported_devices);
    if (index >= communications.size())
    {
        return nullptr;
    }

    auto &communication = communications[index];
    auto device = power_strip_factory::create(std::move(communication));
    if (!device)
    {
        // spdlog::error("std::make_unique() failed creating the device!");
        return nullptr;
    }

    return device;
}

auto sokketter::device(const std::string &serial_number)
    -> const std::unique_ptr<sokketter::power_strip>
{
    auto logger = spdlog::get("libkommpot");
    if (logger != nullptr)
    {
        logger->set_level(spdlog::level::err);
    }

    if (is_internal_testing_enabled())
    {
        return std::make_unique<test_device>();
    }

    const auto supported_devices = power_strip_factory::supported_devices();
    auto communications = kommpot::devices(supported_devices);
    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(std::move(communication));
        if (!device)
        {
            // spdlog::error("std::make_unique() failed creating the device!");
            continue;
        }

        if (device->configuration().id != serial_number)
        {
            continue;
        }

        return device;
    }

    return nullptr;
}
