#include <update_check_storage.h>

#include <sokketter_core.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <json/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <unistd.h>
#endif

auto epoch_seconds_to_timestamp_string(const int64_t epoch_seconds) -> std::string
{
    const auto time_point =
        std::chrono::system_clock::time_point(std::chrono::seconds(epoch_seconds));
    const std::time_t date_time = std::chrono::system_clock::to_time_t(time_point);

    std::tm local_time = {};
#ifdef _WIN32
    localtime_s(&local_time, &date_time);
#else
    localtime_r(&date_time, &local_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

auto parse_update_check_status(const nlohmann::json &j) -> update_check_storage::cached_status
{
    update_check_storage::cached_status r;

    if (j.contains("checked_at") && j["checked_at"].is_string())
    {
        r.status.checked_at = j["checked_at"].get<std::string>();
    }
    else if (j.contains("timestamp") && j["timestamp"].is_string())
    {
        r.status.checked_at = j["timestamp"].get<std::string>();
    }
    else if (j.contains("timestamp") && j["timestamp"].is_number_integer())
    {
        r.status.checked_at = epoch_seconds_to_timestamp_string(j["timestamp"].get<int64_t>());
    }
    else
    {
        r.status.checked_at = "";
    }

    r.status.new_version = j.value("new_version", "");
    r.latest_version = j.value("latest_version", r.status.new_version);

    return r;
}

auto update_check_storage::get() const -> cached_status
{
    return m_result;
}

auto update_check_storage::set(const cached_status &value) -> void
{
    m_result = value;
}

auto update_check_storage::save() const -> void
{
    const auto &destination = path();

    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Saving the update check result to '{}' file.", destination.string());

    std::error_code error_code;
    std::filesystem::create_directories(destination.parent_path(), error_code);

    /**
     * @attention write to a sibling temp file and rename it into place, so a concurrent reader
     * (CLI/UI both use this cache) never observes a truncated or partially written file.
     */
    const auto temporary_path =
        destination.parent_path() /
        (destination.filename().string() + ".tmp." +
            std::to_string(
#ifdef _WIN32
                static_cast<unsigned long>(GetCurrentProcessId())
#else
                static_cast<unsigned long>(getpid())
#endif
                    ) +
            "." + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));

    {
        std::ofstream file(temporary_path.string(), std::ios::trunc);
        if (!file.is_open())
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "Failed opening the update check temp file for writing!");
            return;
        }

        nlohmann::json j = nlohmann::json{{"checked_at", m_result.status.checked_at},
            {"new_version", m_result.status.new_version},
            {"latest_version", m_result.latest_version}};
        file << j.dump(4);

        if (!file.good())
        {
            SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed writing the update check temp file!");
            file.close();
            std::filesystem::remove(temporary_path, error_code);
            return;
        }
    }

    error_code.clear();
#ifdef _WIN32
    if (!MoveFileExW(temporary_path.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        error_code =
            std::error_code(static_cast<int>(GetLastError()), std::system_category());
    }
#else
    std::filesystem::rename(temporary_path, destination, error_code);
#endif
    if (error_code)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
            "Failed replacing '{}' with the update check temp file: {}.", destination.string(),
            error_code.message());
        std::filesystem::remove(temporary_path, error_code);
    }
}

auto update_check_storage::load() -> void
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Restoring the update check result from '{}' file.", path().string());

    m_result = {};

    if (!std::filesystem::exists(path()))
    {
        SPDLOG_LOGGER_INFO(SOKKETTER_LOGGER, "No update check file was found, skipping restoring.");
        return;
    }

    std::ifstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed opening the update check file for reading!");
        return;
    }

    nlohmann::json j;
    try
    {
        file >> j;

        auto value = parse_update_check_status(j);
        m_result = std::move(value);
    }
    catch (const nlohmann::json::exception &exception)
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER,
            "Failed restoring the update check result from '{}' file: {}. Starting with an empty "
            "result.",
            path().string(), exception.what());
    }
}

auto update_check_storage::path() const -> std::filesystem::path
{
    /**
     * @attention lets tests point the cache at a temp file instead of the real storage path.
     */
    const char *test_path = std::getenv("LIBSOKKETTER_TEST_UPDATE_CHECK_PATH");
    if (test_path != nullptr && test_path[0] != '\0')
    {
        return test_path;
    }

    return sokketter::storage_path() / "update-check.json";
}
