#include "core.h"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

core::core()
{
    initialize_logger();

    const auto &shared_data_folder_path = sokketter::storage_path();
    if (!std::filesystem::exists(shared_data_folder_path))
    {
        SPDLOG_DEBUG("No application data storage folder at '{}' was found, creating one.",
            shared_data_folder_path.string());

        std::error_code error_code;
        if (!std::filesystem::create_directories(shared_data_folder_path, error_code))
        {
            SPDLOG_CRITICAL(
                "Failed creating storage directory at '{}'!", shared_data_folder_path.string());
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
            SPDLOG_CRITICAL("Failed creating logs directory at '{}'!", logs_folder_path.string());
        }

        SPDLOG_DEBUG("Folder was created.");
    }
}

core::~core()
{
    deinitialize_logger();
}

auto core::settings() noexcept -> sokketter::settings_information
{
    return m_settings;
}

/**
 * @attention since core is a static member of a namespace, its constructor is called way before
 * entering the main() function of the application (at least on Windows). Thus, creating a global
 * logger here is enough for any linked applications.
 */
auto core::initialize_logger() -> void
{
    const auto logs_folder_path = sokketter::logs_path();
    const auto &filepath = logs_folder_path / (m_logger_name + ".log");

    auto logger = spdlog::daily_logger_mt(m_logger_name, filepath.string(), 0, 0, false, 30);
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%Y-%m-%d %T.%e - %l - %s:%# - %v");

    spdlog::flush_every(std::chrono::seconds(3));

    spdlog::set_default_logger(logger);

    SPDLOG_DEBUG("A new logging session is started.");
}

void core::deinitialize_logger()
{
    SPDLOG_DEBUG("The logging session is finished.");
    spdlog::shutdown();
}
