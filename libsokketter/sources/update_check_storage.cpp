#include <update_check_storage.h>

#include <sokketter_core.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <json/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>

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

auto parse_update_check_status(const nlohmann::json &j) -> sokketter::update_check_status
{
    sokketter::update_check_status r;

    if (j.contains("checked_at") && j["checked_at"].is_string())
    {
        r.checked_at = j["checked_at"].get<std::string>();
    }
    else if (j.contains("timestamp") && j["timestamp"].is_string())
    {
        r.checked_at = j["timestamp"].get<std::string>();
    }
    else if (j.contains("timestamp") && j["timestamp"].is_number_integer())
    {
        r.checked_at = epoch_seconds_to_timestamp_string(j["timestamp"].get<int64_t>());
    }
    else
    {
        r.checked_at = "";
    }

    r.new_version = j.value("new_version", "");
    return r;
}

auto update_check_storage::get() const -> sokketter::update_check_status
{
    return m_result;
}

auto update_check_storage::set(const sokketter::update_check_status &value) -> void
{
    m_result = value;
}

auto update_check_storage::save() const -> void
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Saving the update check result to '{}' file.", path().string());

    std::ofstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed opening the update check file for writing!");
        return;
    }

    nlohmann::json j =
        nlohmann::json{{"checked_at", m_result.checked_at}, {"new_version", m_result.new_version}};
    file << j.dump(4);
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
    return sokketter::storage_path() / "update-check.json";
}
