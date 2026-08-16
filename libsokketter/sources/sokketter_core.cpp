#include "sokketter_core.h"

#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <libsokketter.h>

#include <algorithm>
#include <cctype>
#include <curl/curl.h>
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

namespace {
    struct curl_string_buffer
    {
        std::string data;
    };

    auto write_response_data(char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t
    {
        auto *buffer = static_cast<curl_string_buffer *>(userdata);
        if (buffer == nullptr)
        {
            return 0;
        }

        const auto total_size = size * nmemb;
        buffer->data.append(ptr, total_size);
        return total_size;
    }

    auto normalize_version_string(std::string version) -> std::string
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

    auto parse_version_parts(const std::string &version) -> std::vector<uint32_t>
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
} // namespace

auto sokketter_core::is_newer_version(
    const std::string &current_version, const std::string &candidate_version) -> bool
{
    const auto normalize = [](std::string version) {
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

        return normalized.empty() ? std::string("0.0.0.0") : normalized;
    };

    const auto current_parts = parse_version_parts(normalize(current_version));
    const auto candidate_parts = parse_version_parts(normalize(candidate_version));

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

auto sokketter_core::is_new_version_available(std::string &latest_version) -> bool
{
    latest_version.clear();
    const auto current_version = sokketter::version().to_string();
    const std::string url = "https://api.github.com/repos/morwy/sokketter/releases/latest";

    CURL *curl = curl_easy_init();
    if (curl == nullptr)
    {
        SPDLOG_LOGGER_WARN(SOKKETTER_LOGGER, "Failed to initialize curl for update check.");
        return false;
    }

    curl_string_buffer response = {};
    struct curl_slist *headers = curl_slist_append(nullptr, "Accept: application/vnd.github+json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "sokketter");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    const auto result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (result != CURLE_OK)
    {
        SPDLOG_LOGGER_WARN(
            SOKKETTER_LOGGER, "Failed checking GitHub releases: {}.", curl_easy_strerror(result));
        return false;
    }

    if (response.data.empty())
    {
        return false;
    }

    try
    {
        const auto json_response = nlohmann::json::parse(response.data);
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
