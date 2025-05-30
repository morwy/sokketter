#include "libsokketter.h"

#include <cstdint>
#include <string>
#include <utility>

#include <core.h>
#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <spdlog/spdlog.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

namespace sokketter {
    static core s_core;
} // namespace sokketter

auto sokketter::storage_path() -> std::filesystem::path
{
#ifdef _WIN32
    return "C:\\ProgramData\\sokketter";
#elif __APPLE__
    return "/Users/Shared/sokketter";
#else
    return "~/.local/share/sokketter";
#endif
}

auto sokketter::logs_path() -> std::filesystem::path
{
    return storage_path() / "logs";
}

auto get_requested_test_device_number() -> size_t
{
    const std::string &name = "LIBSOKKETTER_TEST_DEVICE_NUMBER";
    const char *value = std::getenv(name.c_str());
    if (value == nullptr)
    {
        return 0;
    }

    try
    {
        return std::stoul(value);
    }
    catch (const std::invalid_argument &)
    {
        return 0;
    }
    catch (const std::out_of_range &)
    {
        return 0;
    }
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
    return {SOKKETTER_VERSION_MAJOR, SOKKETTER_VERSION_MINOR, SOKKETTER_VERSION_MICRO,
        SOKKETTER_VERSION_NANO, SOKKETTER_VERSION_SHA};
}

auto sokketter::settings() noexcept -> sokketter::settings_information
{
    return s_core.settings();
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

auto sokketter::socket::power(const bool &on) const noexcept -> bool
{
    if (m_power_cb == nullptr)
    {
        SPDLOG_WARN("Trying to change state of socket {} at index {} to {} without set callback!",
            this->configuration().name, this->configuration().id, on ? "on" : "off");
        return false;
    }

    return m_power_cb(m_index, on);
}

auto sokketter::socket::toggle() const noexcept -> bool
{
    if (m_power_cb == nullptr)
    {
        SPDLOG_WARN("Trying to change state of socket {} at index {} without set callback!",
            this->configuration().name, this->configuration().id);
        return false;
    }

    const bool powered_on = is_powered_on();

    return m_power_cb(m_index, !powered_on);
}

auto sokketter::socket::is_powered_on() const noexcept -> bool
{
    if (m_status_cb == nullptr)
    {
        SPDLOG_WARN("Trying to check status of socket {} at index {} without set callback!",
            this->configuration().name, this->configuration().id);
        return false;
    }

    return m_status_cb(m_index);
}

auto sokketter::socket::to_string() const noexcept -> std::string
{
    return this->configuration().name + std::string(", status: ") +
           std::string(is_powered_on() ? "on" : "off");
}

auto sokketter::power_strip_type_to_string(const power_strip_type &type) -> std::string
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
    std::vector<std::unique_ptr<sokketter::power_strip>> devices;

    auto requested_test_device_number = get_requested_test_device_number();
    if (requested_test_device_number > 0)
    {
        SPDLOG_DEBUG("Requested debug devices: {}.", requested_test_device_number);

        for (size_t device_index = 0; device_index < requested_test_device_number; ++device_index)
        {
            devices.push_back(std::make_unique<test_device>(device_index));
        }

        return devices;
    }

    const auto supported_devices = power_strip_factory::supported_devices();

    SPDLOG_DEBUG("Supported devices: {}.", supported_devices.size());

    auto communications = kommpot::devices(supported_devices);

    SPDLOG_DEBUG("Connected devices: {}.", communications.size());

    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(std::move(communication));
        if (!device)
        {
            SPDLOG_ERROR("Failed creating the device - name {}, serial number {}, at port {}!",
                communication->information().name, communication->information().serial_number,
                communication->information().port);
            continue;
        }

        SPDLOG_DEBUG("Device was succesfully created - name {}, serial number {}, at port {}!",
            communication->information().name, communication->information().serial_number,
            communication->information().port);

        devices.push_back(std::move(device));
    }

    SPDLOG_DEBUG("Created devices: {}.", devices.size());

    return devices;
}

auto sokketter::device(const size_t &index) -> const std::unique_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number())
    {
        SPDLOG_DEBUG("Creating debug device at index {}.", index);
        return std::make_unique<test_device>(index);
    }

    const auto supported_devices = power_strip_factory::supported_devices();

    SPDLOG_DEBUG("Supported devices: {}.", supported_devices.size());

    auto communications = kommpot::devices(supported_devices);

    SPDLOG_DEBUG("Connected devices: {}.", communications.size());

    if (index >= communications.size())
    {
        SPDLOG_ERROR("Provided index {} is out of range 0-{}!", index, communications.size());
        return nullptr;
    }

    auto &communication = communications[index];
    auto device = power_strip_factory::create(std::move(communication));
    if (!device)
    {
        SPDLOG_ERROR("Failed creating the device - name {}, serial number {}, at port {}!",
            communication->information().name, communication->information().serial_number,
            communication->information().port);
        return nullptr;
    }

    SPDLOG_DEBUG("Device was succesfully created - name {}, serial number {}, at port {}!",
        communication->information().name, communication->information().serial_number,
        communication->information().port);

    return device;
}

auto sokketter::device(const std::string &serial_number)
    -> const std::unique_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number())
    {
        const std::string test_device_prefix = "TEST_SERIAL_NUMBER_";
        if (serial_number.rfind(test_device_prefix, 0) == 0)
        {
            try
            {
                size_t index = std::stoul(serial_number.substr(test_device_prefix.length()));
                return std::make_unique<test_device>(index);
            }
            catch (const std::invalid_argument &)
            {
                SPDLOG_WARN("Failed reading test device index from the provided serial number {}!",
                    serial_number);
                return nullptr;
            }
            catch (const std::out_of_range &)
            {
                SPDLOG_WARN("Failed reading test device index from the provided serial number {}!",
                    serial_number);
                return nullptr;
            }
        }

        SPDLOG_ERROR("No device was created.");

        return nullptr;
    }

    const auto supported_devices = power_strip_factory::supported_devices();

    SPDLOG_DEBUG("Supported devices: {}.", supported_devices.size());

    auto communications = kommpot::devices(supported_devices);

    SPDLOG_DEBUG("Connected devices: {}.", communications.size());

    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(std::move(communication));
        if (!device)
        {
            SPDLOG_ERROR("Failed creating the device - name {}, serial number {}, at port {}!",
                communication->information().name, communication->information().serial_number,
                communication->information().port);
            continue;
        }

        if (device->configuration().id != serial_number)
        {
            continue;
        }

        SPDLOG_DEBUG("Device was succesfully created - name {}, serial number {}, at port {}!",
            communication->information().name, communication->information().serial_number,
            communication->information().port);

        return device;
    }

    SPDLOG_ERROR("No device was created.");

    return nullptr;
}
