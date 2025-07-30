#include "sokketter_core.h"

#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <devices/test_device.h>
#include <libsokketter.h>

#include <spdlog/sinks/callback_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
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
    m_database.release_resources();

    kommpot::deinitialize();

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

    const auto supported_devices = power_strip_factory::supported_devices();

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
