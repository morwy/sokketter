#include <app_settings_storage.h>

#include <app_logger.h>
#include <libsokketter.h>

#include <QFileInfo>
#include <fstream>
#include <spdlog/spdlog.h>

auto app_settings_storage::get() -> app_settings &
{
    return m_settings;
}

auto app_settings_storage::set(const app_settings &settings) -> void
{
    m_settings = settings;
}

auto app_settings_storage::save() const -> void
{
    SPDLOG_LOGGER_DEBUG(
        APP_LOGGER, "Saving the application settings to '{}' file.", path().string());

    std::ofstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed saving the application settings!");
        return;
    }

    nlohmann::json j = m_settings;
    file << j.dump(4);
}

auto app_settings_storage::load() -> void
{
    SPDLOG_LOGGER_DEBUG(
        APP_LOGGER, "Restoring the application settings from '{}' file.", path().string());

    m_settings = {};

    QFileInfo fileInfo(path());
    if (!fileInfo.exists())
    {
        SPDLOG_LOGGER_INFO(
            APP_LOGGER, "No application settings file was found, skipping restoring.");
        return;
    }

    std::ifstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed restoring the application settings!");
        return;
    }

    nlohmann::json j;
    try
    {
        file >> j;

        auto settings = j.get<app_settings>();
        m_settings = std::move(settings);
    }
    catch (const nlohmann::json::exception &exception)
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER,
            "Failed restoring the application settings from '{}' file: {}. Starting with "
            "default settings.",
            path().string(), exception.what());
    }
}

auto app_settings_storage::path() const -> std::filesystem::path
{
    return sokketter::storage_path() / "sokketter-ui.json";
}
