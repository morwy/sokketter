#include <app_logger.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#    include <spdlog/sinks/msvc_sink.h>
#endif

#ifdef __linux__
#    include <spdlog/sinks/syslog_sink.h>
#endif

#ifdef __APPLE__
#    include <spdlog/sinks/syslog_sink.h>
#endif

auto initialize_app_logger(const std::filesystem::path &path) -> void
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
    auto syslog_sink =
        std::make_shared<spdlog::sinks::syslog_sink_mt>(LOGGER_NAME, 0, LOG_USER, false);
    new_sinks.push_back(syslog_sink);
#endif

    const auto &filepath = path / (std::string(LOGGER_NAME) + ".txt");

    auto logger = spdlog::daily_logger_mt(LOGGER_NAME, filepath.string(), 0, 0, false, 30);

    APP_LOGGER->set_level(spdlog::level::trace);
    APP_LOGGER->set_pattern("%Y-%m-%d %T.%e - %l - %s:%# - %v");

    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "A new logging session is started.");
}

auto deinitialize_app_logger() -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "The logging session is finished.");
    APP_LOGGER->flush();
    spdlog::drop(LOGGER_NAME);
}
