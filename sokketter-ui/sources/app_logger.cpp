#include <app_logger.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>

#ifdef _WIN32
#    include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef __linux__
#    include <spdlog/sinks/syslog_sink.h>
#endif

#ifdef __APPLE__
#    include <spdlog/sinks/rotating_file_sink.h>
#endif

auto initialize_app_logger() -> void
{
    std::vector<spdlog::sink_ptr> new_sinks;

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
    constexpr const auto maxFileCount = 10;
    constexpr const auto maxFileSize = 100 * 1024 * 1024;

    const auto consoleLogPath =
        std::filesystem::path(getenv("HOME")) / "Library" / "Logs" / "sokketter";
    const auto filename = "sokketter-ui.log";
    const auto logFilepath = consoleLogPath / filename;

    auto macosConsoleLogger = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logFilepath, maxFileSize, maxFileCount, true);

    new_sinks.push_back(macosConsoleLogger);
#endif

    const auto &filepath = sokketter::storage_path() / "logs" / (std::string(LOGGER_NAME) + ".txt");

    auto logger = spdlog::daily_logger_mt(LOGGER_NAME, filepath.string(), 0, 0, false, 30);
    APP_LOGGER->sinks().insert(logger->sinks().end(), new_sinks.begin(), new_sinks.end());

    APP_LOGGER->set_level(spdlog::level::trace);
    APP_LOGGER->set_pattern("%Y-%m-%d %T.%e - %l - %s:%# - %v");

    sokketter::settings_structure settings;

    settings.logging_level = sokketter::logging_level(APP_LOGGER->level());
    settings.logging_callback = std::bind(&logging_callback, std::placeholders::_1);

    sokketter::set_settings(settings);

    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "A new logging session is started.");
}

auto deinitialize_app_logger() -> void
{
    if (APP_LOGGER == nullptr)
    {
        return;
    }

    sokketter::settings_structure settings;
    settings.logging_level = sokketter::logging_level::OFF;
    sokketter::set_settings(settings);

    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "The logging session is finished.");
    APP_LOGGER->flush();
    spdlog::drop(LOGGER_NAME);
}

auto logging_callback(const sokketter::callback_response_structure &response) -> void
{
    APP_LOGGER->log(spdlog::source_loc{response.file, response.line, response.function},
        spdlog::level::level_enum(response.level), response.message);
}
