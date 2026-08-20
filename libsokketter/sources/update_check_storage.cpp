#include <update_check_storage.h>

#include <libsokketter.h>
#include <sokketter_core.h>

#include <fstream>
#include <json/json.hpp>
#include <spdlog/spdlog.h>

void to_json(nlohmann::json &j, const update_check_storage::result &r)
{
    j = nlohmann::json{{"timestamp", r.timestamp}, {"new_version", r.new_version}};
}

void from_json(const nlohmann::json &j, update_check_storage::result &r)
{
    r.timestamp = j.value("timestamp", static_cast<int64_t>(0));
    r.new_version = j.value("new_version", "");
}

auto update_check_storage::get() const -> result
{
    return m_result;
}

auto update_check_storage::set(const result &value) -> void
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

    nlohmann::json j = m_result;
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

        auto value = j.get<result>();
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
