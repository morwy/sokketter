#include "libsokketter.h"

#include <cstdint>
#include <string>
#include <utility>

#include <database_storage.h>
#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <sokketter_core.h>
#include <spdlog/spdlog.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

auto sokketter::initialize() -> bool
{
    return sokketter_core::instance().initialize();
}

auto sokketter::deinitialize() -> bool
{
    return sokketter_core::instance().deinitialize();
}

auto sokketter::settings() noexcept -> sokketter::settings_structure
{
    return sokketter_core::instance().settings();
}

auto sokketter::set_settings(const settings_structure &settings) noexcept -> void
{
    sokketter_core::instance().set_settings(settings);
}

auto sokketter::storage_path() -> std::filesystem::path
{
#ifdef _WIN32
    return "C:\\ProgramData\\sokketter";
#elif __APPLE__
    return "/Users/Shared/sokketter";
#else
    return std::string(getenv("HOME")) + "/.local/share/sokketter";
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

sokketter::socket::socket(const socket_configuration &configuration)
{
    m_configuration = configuration;
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
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
            "Trying to change state of socket {} to {} without set callback!",
            this->configuration().name, on ? "on" : "off");
        return false;
    }

    return m_power_cb(m_index, on);
}

auto sokketter::socket::toggle() const noexcept -> bool
{
    if (m_power_cb == nullptr)
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
            "Trying to change state of socket {} without set callback!",
            this->configuration().name);
        return false;
    }

    const bool powered_on = is_powered_on();

    return m_power_cb(m_index, !powered_on);
}

auto sokketter::socket::is_powered_on() const noexcept -> bool
{
    if (m_status_cb == nullptr)
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
            "Trying to check status of socket {} without set callback!",
            this->configuration().name);
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
    case power_strip_type::ENERGENIE_EG_PMXX_LAN: {
        return "Energenie EG-PMxx-LAN";
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

void sokketter::power_strip::save()
{
    sokketter_core::instance().database().save();
}

auto sokketter::power_strip::is_connected() const -> bool
{
    return false;
}

auto sokketter::power_strip::sockets() -> std::vector<sokketter::socket> &
{
    return m_sockets;
}

auto sokketter::power_strip::sockets() const -> const std::vector<sokketter::socket> &
{
    return m_sockets;
}

auto sokketter::power_strip::socket(const size_t &index)
    -> const std::optional<std::reference_wrapper<sokketter::socket>>
{
    return std::nullopt;
}

auto sokketter::power_strip::to_string() const noexcept -> std::string
{
    return this->configuration().name + " (" +
           power_strip_type_to_string(this->configuration().type) + ", " +
           this->configuration().id + ", located at " + this->configuration().address + ")";
}

auto sokketter::devices(const device_filter &filter)
    -> const std::vector<std::shared_ptr<sokketter::power_strip>> &
{
    auto requested_test_device_number = get_requested_test_device_number();
    if (requested_test_device_number > 0)
    {
        static std::vector<std::shared_ptr<sokketter::power_strip>> devices;

        devices.clear();

        SPDLOG_LOGGER_DEBUG(
            SOKKETTER_LOGGER, "Requested debug devices: {}.", requested_test_device_number);

        for (size_t device_index = 0; device_index < requested_test_device_number; ++device_index)
        {
            devices.push_back(std::make_shared<test_device>(device_index));
        }

        return devices;
    }

    return sokketter_core::instance().devices(filter);
}

auto sokketter::device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number())
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Creating debug device at index {}.", index);
        return std::make_shared<test_device>(index);
    }

    auto &devices = sokketter_core::instance().devices();

    if (index >= devices.size())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
            "Failed creating the device - requested index {} is greater that the number of the "
            "devices ({})!",
            index, devices.size());
        return {};
    }

    return devices[index];
}

auto sokketter::device(const std::string &serial_number) -> std::shared_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number())
    {
        const std::string test_device_prefix = "TEST_SERIAL_NUMBER_";
        if (serial_number.rfind(test_device_prefix, 0) == 0)
        {
            try
            {
                size_t index = std::stoul(serial_number.substr(test_device_prefix.length()));
                return std::make_shared<test_device>(index);
            }
            catch (const std::invalid_argument &)
            {
                SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
                    "Failed reading test device index from the provided serial number {}!",
                    serial_number);
                return nullptr;
            }
            catch (const std::out_of_range &)
            {
                SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
                    "Failed reading test device index from the provided serial number {}!",
                    serial_number);
                return nullptr;
            }
        }

        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "No device was created.");

        return nullptr;
    }

    auto &devices = sokketter_core::instance().devices();

    for (const auto &device : devices)
    {
        if (device && device->configuration().id == serial_number)
        {
            return device;
        }
    }

    SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER, "No device found with serial number {}.", serial_number);

    return nullptr;
}
