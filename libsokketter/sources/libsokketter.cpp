#include "libsokketter.h"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <optional>
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

// The upper bound on the number of fake test devices that may be requested through the
// LIBSOKKETTER_TEST_DEVICE_NUMBER environment variable. Keeps the reserve() call in
// sokketter::devices() provably bounded.
static constexpr size_t LIBSOKKETTER_MAX_TEST_DEVICE_NUMBER = 1000;

auto get_requested_test_device_number() -> std::optional<size_t>
{
    const char *value = std::getenv("LIBSOKKETTER_TEST_DEVICE_NUMBER");
    if (value == nullptr)
    {
        return std::nullopt;
    }

    size_t number = 0;
    const auto *end = value + std::strlen(value);
    const auto [ptr, ec] = std::from_chars(value, end, number);
    if (ec != std::errc{} || ptr != end || number > LIBSOKKETTER_MAX_TEST_DEVICE_NUMBER)
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER,
            "Ignoring invalid LIBSOKKETTER_TEST_DEVICE_NUMBER value: {}.", value);
        return std::nullopt;
    }

    return number;
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

auto sokketter::release_link() -> std::string
{
    return sokketter_core::instance().release_link();
}

auto sokketter::is_new_release_available(std::string &latest_version) -> bool
{
    return sokketter_core::instance().is_new_release_available(latest_version);
}

auto sokketter::check_for_update_async() -> void
{
    sokketter_core::instance().check_for_update_async();
}

auto sokketter::last_update_check_status() -> update_check_status
{
    return sokketter_core::instance().last_update_check_status();
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

auto sokketter::power_strip_authentication_type_to_string(
    const power_strip_authentication_type &type) -> std::string
{
    switch (type)
    {
    case power_strip_authentication_type::NONE: {
        return "None";
    }
    case power_strip_authentication_type::PASSWORD_ONLY: {
        return "Password only";
    }
    default: {
        return "Unknown";
    }
    }
}

auto sokketter::power_strip_authentication::is_valid() const -> bool
{
    switch (type)
    {
    default:
    case power_strip_authentication_type::UNKNOWN: {
        return false;
    }
    case power_strip_authentication_type::NONE: {
        return true;
    }
    case power_strip_authentication_type::PASSWORD_ONLY: {
        return !password.empty();
    }
    }
}

auto sokketter::power_strip::configuration() const -> power_strip_configuration
{
    return m_configuration;
}

auto sokketter::power_strip::configure(const power_strip_configuration &configuration) -> void
{
    m_configuration = configuration;
}

auto sokketter::power_strip::save() const -> void
{
    sokketter_core::instance().save_database();
}

auto sokketter::power_strip::try_authenticate() -> bool
{
    return false;
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
    const auto configuration = this->configuration();

    std::string text = configuration.name + " (" + power_strip_type_to_string(configuration.type) +
                       ", " + configuration.id;

    if (configuration.address.empty())
    {
        text += ", disconnected)";
    }
    else
    {
        text += ", available at " + configuration.address + ")";
    }

    return text;
}

auto sokketter::devices(const device_filter &filter)
    -> std::vector<std::shared_ptr<sokketter::power_strip>>
{
    if (const auto requested_test_device_number = get_requested_test_device_number();
        requested_test_device_number.has_value())
    {
        std::vector<std::shared_ptr<sokketter::power_strip>> devices;
        devices.reserve(*requested_test_device_number);

        SPDLOG_LOGGER_DEBUG(
            SOKKETTER_LOGGER, "Requested debug devices: {}.", *requested_test_device_number);

        for (size_t device_index = 0; device_index < *requested_test_device_number; ++device_index)
        {
            devices.push_back(std::make_shared<test_device>(device_index));
        }

        return devices;
    }

    return sokketter_core::instance().devices(filter);
}

auto sokketter::devices(
    const device_filter &filter, device_callback device_cb, status_callback status_cb) -> void
{
    if (const auto requested_test_device_number = get_requested_test_device_number();
        requested_test_device_number.has_value())
    {
        std::vector<std::shared_ptr<sokketter::power_strip>> devices;
        devices.reserve(*requested_test_device_number);

        SPDLOG_LOGGER_DEBUG(
            SOKKETTER_LOGGER, "Requested debug devices: {}.", *requested_test_device_number);

        for (size_t device_index = 0; device_index < *requested_test_device_number; ++device_index)
        {
            devices.push_back(std::make_shared<test_device>(device_index));
        }

        status_cb(enumeration_status::LOOKING_FOR_NEW_USB_DEVICES);
        status_cb(enumeration_status::LOOKING_FOR_NEW_ETHERNET_DEVICES);

        device_cb(devices);

        status_cb(enumeration_status::COMPLETED);

        return;
    }

    sokketter_core::instance().devices(filter, device_cb, status_cb);
}

auto sokketter::device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number().has_value())
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Creating debug device at index {}.", index);
        return std::make_shared<test_device>(index);
    }

    return sokketter_core::instance().device(index);
}

auto sokketter::device(const std::string &serial_number) -> std::shared_ptr<sokketter::power_strip>
{
    if (get_requested_test_device_number().has_value())
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

    return sokketter_core::instance().device(serial_number);
}

auto sokketter::enumeration_status_to_string(const enumeration_status &status) noexcept
    -> std::string
{
    switch (status)
    {
    case enumeration_status::UNKNOWN: {
        return "Unknown";
    }
    case enumeration_status::LOOKING_FOR_NEW_USB_DEVICES: {
        return "Looking for the new USB devices";
    }
    case enumeration_status::LOOKING_FOR_KNOWN_ETHERNET_DEVICES: {
        return "Looking for the known Ethernet devices";
    }
    case enumeration_status::LOOKING_FOR_NEW_ETHERNET_DEVICES: {
        return "Looking for the new Ethernet devices";
    }
    case enumeration_status::COMPLETED: {
        return "Completed";
    }
    default:
        return "";
    }
}

auto sokketter::forget_device(std::shared_ptr<power_strip> &device) -> void
{
    sokketter_core::instance().remove_device(device);
}
