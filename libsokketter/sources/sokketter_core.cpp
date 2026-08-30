#include "sokketter_core.h"

#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <libsokketter.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <json/json.hpp>
#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

#ifdef _WIN32
#    include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef __linux__
#    include <spdlog/sinks/syslog_sink.h>
#endif

#ifdef __APPLE__
#    include <spdlog/sinks/syslog_sink.h>
#endif

auto sokketter_core::initialize() -> bool
{
    const auto &shared_data_folder_path = sokketter::storage_path();
    if (!std::filesystem::exists(shared_data_folder_path))
    {
        SPDLOG_DEBUG("No application data storage folder at '{}' was found, creating one.",
            shared_data_folder_path.string());

        std::error_code error_code;
        if (!std::filesystem::create_directories(shared_data_folder_path, error_code))
        {
            SPDLOG_CRITICAL("Failed creating storage directory at '{}' with error '{}'!",
                shared_data_folder_path.string(), error_code.message());
            return false;
        }

        SPDLOG_DEBUG("Folder was created.");
    }

    const auto &logs_folder_path = sokketter::logs_path();
    if (!std::filesystem::exists(logs_folder_path))
    {
        SPDLOG_DEBUG("No logs folder at '{}' was found, creating one.", logs_folder_path.string());

        std::error_code error_code;
        if (!std::filesystem::create_directories(logs_folder_path, error_code))
        {
            SPDLOG_CRITICAL("Failed creating logs directory at '{}' with error '{}'!",
                logs_folder_path.string(), error_code.message());
            return false;
        }

        SPDLOG_DEBUG("Folder was created.");
    }

    initialize_logger();

    if (!kommpot::initialize())
    {
        SPDLOG_CRITICAL("Failed initializing kommpot library!");
        return false;
    }

    m_database.load();

    return true;
}

auto sokketter_core::deinitialize() -> bool
{
    {
        const std::lock_guard<std::mutex> lock(m_update_check_thread_mutex);
        if (m_update_check_thread.joinable())
        {
            m_update_check_thread.join();
        }
    }

    m_database.save();

    /**
     * @brief wait for any ongoing device enumeration to finish before releasing the devices,
     *        otherwise the enumeration thread accesses the database while it is being cleared.
     */
    kommpot::deinitialize();

    m_database.release_resources();

    deinitialize_logger();

    return true;
}

auto sokketter_core::settings() noexcept -> sokketter::settings_structure
{
    return m_settings;
}

auto sokketter_core::set_settings(const sokketter::settings_structure &settings) noexcept -> void
{
    m_settings = settings;

    if (m_settings.logging_level == sokketter::logging_level::OFF)
    {
        deinitialize_logger();
    }
    else
    {
        initialize_logger();
    }
}

auto sokketter_core::database() -> database_storage &
{
    return m_database;
}

auto sokketter_core::devices(const sokketter::device_filter &filter)
    -> const std::vector<std::shared_ptr<sokketter::power_strip>> &
{
    auto &database = sokketter_core::instance().database().get();

    const auto supported_devices = power_strip_factory::supported_devices(filter);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Supported devices: {}.", supported_devices.size());

    auto communications = kommpot::devices(supported_devices);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Connected devices: {}.", communications.size());

    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(communication);
        if (!device)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed creating the device!");
            continue;
        }

        auto baseDevice = dynamic_cast<sokketter::power_strip *>(device.get());
        if (baseDevice == nullptr)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
                "{}: failed casting the device to power_strip_base!", device->to_string());
            continue;
        }

        /**
         * @brief look for saved configuration of this device.
         */
        auto it = std::find_if(database.begin(), database.end(),
            [&](const std::shared_ptr<sokketter::power_strip> &item) {
                return item->configuration().id == device->configuration().id;
            });

        if (it != database.end())
        {
            auto baseIt = dynamic_cast<power_strip_base *>(it->get());
            if (baseIt == nullptr)
            {
                SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
                    "{}: failed casting the device to power_strip_base!", device->to_string());
                continue;
            }

            baseIt->initialize(communication);

            SPDLOG_LOGGER_DEBUG(
                SOKKETTER_LOGGER, "{}: device was successfully created!", device->to_string());
        }
        else
        {
            /**
             * @brief append basic device configuration if it is a first time.
             */
            SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
                "{}: new device was successfully created and added to database!",
                device->to_string());

            database.push_back(device);

            sokketter_core::instance().database().save();
        }
    }

    /**
     * Sort the database by device name.
     */
    std::sort(database.begin(), database.end(),
        [](const std::shared_ptr<sokketter::power_strip> &a,
            const std::shared_ptr<sokketter::power_strip> &b) {
            return a->configuration().name < b->configuration().name;
        });

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Created devices: {}.", database.size());

    return database;
}

auto sokketter_core::devices(const sokketter::device_filter &filter,
    sokketter::device_callback device_cb, sokketter::status_callback status_cb) -> void
{
    m_device_cb = device_cb;
    m_status_cb = status_cb;

    const auto supported_devices = power_strip_factory::supported_devices(filter);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Supported devices: {}.", supported_devices.size());

    kommpot::devices(supported_devices,
        std::bind(&sokketter_core::new_devices_received, this, std::placeholders::_1),
        std::bind(&sokketter_core::new_status_received, this, std::placeholders::_1));
}

auto sokketter_core::device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>
{
    auto &database = sokketter_core::instance().database().get();

    if (index >= database.size())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
            "Failed creating the device - requested index {} is greater that the number of the "
            "devices ({})!",
            index, database.size());
        return nullptr;
    }

    return database[index];
}

auto sokketter_core::device(const std::string &serial_number)
    -> std::shared_ptr<sokketter::power_strip>
{
    auto &database = sokketter_core::instance().database().get();

    for (const auto &device : database)
    {
        if (device && device->configuration().id == serial_number)
        {
            return device;
        }
    }

    SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER, "No device found with serial number {}.", serial_number);

    return nullptr;
}

auto sokketter_core::normalize_version_string(std::string version) -> std::string
{
    while (!version.empty() && (version.front() == 'v' || version.front() == 'V'))
    {
        version.erase(version.begin());
    }

    std::string normalized;
    normalized.reserve(version.size());
    for (const char ch : version)
    {
        if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.')
        {
            normalized.push_back(ch);
        }
    }

    return normalized.empty() ? "0.0.0.0" : normalized;
}

auto sokketter_core::parse_version_parts(const std::string &version) -> std::vector<uint32_t>
{
    std::vector<uint32_t> parts;
    std::stringstream stream(normalize_version_string(version));
    std::string part;

    while (std::getline(stream, part, '.'))
    {
        if (part.empty())
        {
            continue;
        }

        try
        {
            parts.push_back(static_cast<uint32_t>(std::stoul(part)));
        }
        catch (const std::exception &)
        {
            parts.push_back(0u);
        }
    }

    while (parts.size() < 4)
    {
        parts.push_back(0u);
    }

    return parts;
}

auto sokketter_core::is_newer_version(
    const std::string &current_version, const std::string &candidate_version) -> bool
{
    const auto current_parts = parse_version_parts(current_version);
    const auto candidate_parts = parse_version_parts(candidate_version);

    for (size_t i = 0; i < std::max(current_parts.size(), candidate_parts.size()); ++i)
    {
        const auto current_part = i < current_parts.size() ? current_parts[i] : 0u;
        const auto candidate_part = i < candidate_parts.size() ? candidate_parts[i] : 0u;

        if (candidate_part > current_part)
        {
            return true;
        }

        if (candidate_part < current_part)
        {
            return false;
        }
    }

    return false;
}

auto sokketter_core::release_link() -> std::string
{
    return RELEASE_LINK;
}

auto current_timestamp() -> std::string
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &now_time);
#else
    localtime_r(&now_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

auto sokketter_core::check_for_update_async() -> void
{
    /**
     * @attention lets tests stub out the real GitHub request.
     */
    if (std::getenv("LIBSOKKETTER_TEST_SKIP_UPDATE_CHECK") != nullptr)
    {
        return;
    }

    const std::lock_guard<std::mutex> lock(m_update_check_thread_mutex);

    if (m_update_check_running.load())
    {
        SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
            "Skipping update check request because another check is already running.");
        return;
    }

    if (m_update_check_thread.joinable())
    {
        m_update_check_thread.join();
    }

    m_update_check_running.store(true);

    m_update_check_thread = std::thread([this]() {
        struct update_check_running_guard
        {
            std::atomic_bool &running;
            ~update_check_running_guard()
            {
                running.store(false);
            }
        };

        update_check_running_guard running_guard{m_update_check_running};

        std::string latest_version;
        const bool has_update = is_new_release_available(latest_version);
        if (!has_update && latest_version.empty())
        {
            return;
        }

        update_check_storage::cached_status result;
        result.status.checked_at = current_timestamp();
        result.status.new_version = has_update ? latest_version : std::string();
        result.latest_version = latest_version;

        const std::lock_guard<std::mutex> lock(m_update_check_storage_mutex);
        m_update_check_storage.set(result);
        m_update_check_storage.save();
    });
}

auto sokketter_core::last_update_check_status() -> sokketter::update_check_status
{
    const std::lock_guard<std::mutex> lock(m_update_check_storage_mutex);

    m_update_check_storage.load();
    auto result = m_update_check_storage.get();

    if (!result.latest_version.empty())
    {
        const auto current_version = sokketter::version().to_string();
        result.status.new_version =
            is_newer_version(current_version, result.latest_version) ? result.latest_version : "";
    }

    return result.status;
}

auto sokketter_core::is_new_release_available(std::string &latest_version) -> bool
{
    latest_version.clear();
    const auto current_version = sokketter::version().to_string();

    kommpot::http_device_identification identification;
    identification.address = RELEASE_API_HOST;
    identification.port = 443;
    identification.use_tls = true;

    auto communication = kommpot::device(identification);
    if (communication == nullptr)
    {
        SPDLOG_LOGGER_WARN(
            SOKKETTER_LOGGER, "Failed to create the HTTP communication for update check.");
        return false;
    }

    kommpot::http_device_configuration communication_configuration;
    communication_configuration.timeout_ms = UPDATE_CHECK_TIMEOUT_MSECS;
    communication_configuration.user_agent = "sokketter";
    communication->set_configuration(communication_configuration);

    if (!communication->open())
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER, "Failed to open the HTTP session for update check.");
        return false;
    }

    kommpot::http_transfer_configuration http_configuration;
    http_configuration.type = kommpot::http_transfer_type::GET;
    http_configuration.resource_path = RELEASE_API_PATH;
    http_configuration.headers = {{"Accept", "application/vnd.github+json"}};

    kommpot::transfer_configuration configuration = http_configuration;

    std::string response = "";
    if (communication->write(configuration, nullptr, 0))
    {
        const auto *performed_configuration =
            std::get_if<kommpot::http_transfer_configuration>(&configuration);

        std::array<char, RESPONSE_CHUNK_SIZE_BYTES> chunk = {};
        while (performed_configuration != nullptr &&
               communication->read(configuration, chunk.data(), chunk.size()))
        {
            response.append(chunk.data(), performed_configuration->bytes_read);
        }
    }
    else
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER, "Failed checking GitHub releases.");
    }

    communication->close();

    if (response.empty())
    {
        return false;
    }

    try
    {
        const auto json_response = nlohmann::json::parse(response);
        if (!json_response.contains("tag_name") || !json_response["tag_name"].is_string())
        {
            return false;
        }

        latest_version = json_response["tag_name"].get<std::string>();
    }
    catch (const nlohmann::json::exception &exception)
    {
        SPDLOG_LOGGER_WARN(
            SOKKETTER_LOGGER, "Failed parsing GitHub release response: {}.", exception.what());
        return false;
    }

    if (latest_version.empty())
    {
        return false;
    }

    return is_newer_version(current_version, latest_version);
}

auto sokketter_core::initialize_logger() -> void
{
    std::vector<spdlog::sink_ptr> new_sinks;

    if (m_settings.logging_callback != nullptr)
    {
        /**
         * @brief initialize only callback functionality.
         */
        auto callback_sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [](const spdlog::details::log_msg &msg) {
                const auto &settings = sokketter_core::instance().settings();
                if (settings.logging_callback != nullptr)
                {
                    settings.logging_callback(sokketter::callback_response_structure{
                        sokketter::logging_level(msg.level), msg.source.filename, msg.source.line,
                        msg.source.funcname, std::string(msg.payload.data(), msg.payload.size())});
                }
            });
        new_sinks.push_back(callback_sink);
    }
    else
    {
        /**
         * @brief initialize default functionality.
         */
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        new_sinks.push_back(console_sink);

#ifdef _WIN32
        auto msvc_qt_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        new_sinks.push_back(msvc_qt_sink);
#endif

#ifdef __linux__
        auto syslog_sink =
            std::make_shared<spdlog::sinks::syslog_sink_mt>(LOGGER_NAME, 0, LOG_USER, false);
        new_sinks.push_back(syslog_sink);
#endif

#ifdef __APPLE__
        auto syslog_sink =
            std::make_shared<spdlog::sinks::syslog_sink_mt>(LOGGER_NAME, 0, LOG_USER, false);
        new_sinks.push_back(syslog_sink);
#endif
    }

    if (m_logger == nullptr)
    {
        m_logger =
            std::make_shared<spdlog::logger>(LOGGER_NAME, new_sinks.begin(), new_sinks.end());
        spdlog::register_logger(m_logger);
    }
    else
    {
        m_logger->flush();
        spdlog::drop(LOGGER_NAME);

        m_logger =
            std::make_shared<spdlog::logger>(LOGGER_NAME, new_sinks.begin(), new_sinks.end());
        spdlog::register_logger(m_logger);
    }

    SOKKETTER_LOGGER->set_level(spdlog::level::level_enum(m_settings.logging_level));
    SOKKETTER_LOGGER->set_pattern(m_settings.logging_pattern);

    kommpot::settings_structure settings;

    settings.logging_level = kommpot::logging_level(m_settings.logging_level);
    settings.logging_callback =
        std::bind(&sokketter_core::logging_callback, this, std::placeholders::_1);

    kommpot::set_settings(settings);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "A new logging session is started.");
}

auto sokketter_core::deinitialize_logger() -> void
{
    if (SOKKETTER_LOGGER == nullptr)
    {
        return;
    }

    kommpot::settings_structure settings;
    settings.logging_level = kommpot::logging_level::OFF;
    kommpot::set_settings(settings);

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "The logging session is finished.");
    SOKKETTER_LOGGER->flush();
    spdlog::drop(LOGGER_NAME);
}

auto sokketter_core::logging_callback(const kommpot::callback_response_structure &response) -> void
{
    if (SOKKETTER_LOGGER == nullptr)
    {
        return;
    }

    SOKKETTER_LOGGER->log(spdlog::source_loc{response.file, response.line, response.function},
        spdlog::level::level_enum(response.level), response.message);
}

auto sokketter_core::new_devices_received(
    std::vector<std::shared_ptr<kommpot::device_communication>> communications) -> void
{
    auto &database = sokketter_core::instance().database().get();

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Connected devices: {}.", communications.size());

    for (auto &communication : communications)
    {
        auto device = power_strip_factory::create(communication);
        if (!device)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed creating the device!");
            continue;
        }

        auto baseDevice = dynamic_cast<sokketter::power_strip *>(device.get());
        if (baseDevice == nullptr)
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
                "{}: failed casting the device to power_strip_base!", device->to_string());
            continue;
        }

        /**
         * @brief look for saved configuration of this device.
         */
        auto it = std::find_if(database.begin(), database.end(),
            [&](const std::shared_ptr<sokketter::power_strip> &item) {
                return item && item->configuration().id == device->configuration().id;
            });

        if (it != database.end())
        {
            auto baseIt = dynamic_cast<power_strip_base *>(it->get());
            if (baseIt == nullptr)
            {
                SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
                    "{}: failed casting the device to power_strip_base!", device->to_string());
                continue;
            }

            baseIt->initialize(communication);

            SPDLOG_LOGGER_DEBUG(
                SOKKETTER_LOGGER, "{}: device was successfully created!", device->to_string());
        }
        else
        {
            /**
             * @brief append basic device configuration if it is a first time.
             */
            SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER,
                "{}: new device was successfully created and added to database!",
                device->to_string());

            database.push_back(device);

            sokketter_core::instance().database().save();
        }
    }

    /**
     * Sort the database by device name.
     */
    std::sort(database.begin(), database.end(),
        [](const std::shared_ptr<sokketter::power_strip> &a,
            const std::shared_ptr<sokketter::power_strip> &b) {
            return a->configuration().name < b->configuration().name;
        });

    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Created devices: {}.", database.size());

    m_device_cb(database);
}

auto sokketter_core::new_status_received(kommpot::enumeration_status status) -> void
{
    m_status_cb(static_cast<sokketter::enumeration_status>(status));
}
