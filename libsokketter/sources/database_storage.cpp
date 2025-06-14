#include <database_storage.h>

#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <sokketter_core.h>

#include <fstream>
#include <json/json.hpp>
#include <spdlog/spdlog.h>

namespace sokketter {
    void to_json(nlohmann::json &j, const sokketter::socket &s)
    {
        j = nlohmann::json{{"id", s.configuration().id}, {"name", s.configuration().name},
            {"description", s.configuration().description}};
    }

    void from_json(const nlohmann::json &j, sokketter::socket &s)
    {
        auto configuration = s.configuration();

        configuration.id = j.value("id", "");
        configuration.name = j.value("name", "");
        configuration.description = j.value("description", "");

        s.configure(configuration);
    }

    NLOHMANN_JSON_SERIALIZE_ENUM(sokketter::power_strip_type,
        {
            {sokketter::power_strip_type::UNKNOWN, nullptr},
            {sokketter::power_strip_type::ENERGENIE_EG_PMS, "ENERGENIE-EG-PMS"},
            {sokketter::power_strip_type::ENERGENIE_EG_PMS2, "ENERGENIE-EG-PMS2"},
            {sokketter::power_strip_type::GEMBIRD_SIS_PM, "GEMBIRD-SIS-PM"},
            {sokketter::power_strip_type::GEMBIRD_MSIS_PM, "GEMBIRD-MSIS-PM"},
            {sokketter::power_strip_type::GEMBIRD_MSIS_PM_2, "GEMBIRD-MSIS-PM2"},
            {sokketter::power_strip_type::TEST_DEVICE, "TEST-DEVICE"},
        })

    void to_json(nlohmann::json &j, const sokketter::power_strip &ps)
    {
        j = nlohmann::json{{"type", ps.configuration().type}, {"id", ps.configuration().id},
            {"name", ps.configuration().name}, {"description", ps.configuration().description},
            {"sockets", ps.sockets()}};
    }

    void from_json(const nlohmann::json &j, sokketter::power_strip &ps)
    {
        auto configuration = ps.configuration();

        configuration.type = j.value("type", sokketter::power_strip_type::UNKNOWN);
        configuration.id = j.value("id", "");
        configuration.name = j.value("name", "");
        configuration.description = j.value("description", "");

        ps.configure(configuration);

        // ps. j.value("sockets", {});
    }

    void to_json(nlohmann::json &j, const std::unique_ptr<sokketter::power_strip> &ptr)
    {
        if (ptr)
        {
            j = *ptr;
        }
        else
        {
            j = nullptr;
        }
    }

    void from_json(const nlohmann::json &j, std::unique_ptr<sokketter::power_strip> &ptr)
    {
        if (j.is_null())
        {
            ptr = nullptr;
        }
        else
        {
            // ptr = std::make_unique<power_strip_base>(j.get<power_strip_base>());
            const auto &power_strip = j.get<sokketter::power_strip>();
            ptr = power_strip_factory::create(power_strip.configuration().type);
        }
    }
} // namespace sokketter

auto database_storage::get() -> std::vector<std::unique_ptr<sokketter::power_strip>> &
{
    return m_database;
}

auto database_storage::save() const -> void
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Saving the device database to '{}' file.", path().string());

    std::ofstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed saving the device database!");
        return;
    }

    nlohmann::json j = m_database;
    file << j.dump(4);
}

auto database_storage::load() -> void
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Restoring the device database from '{}' file.", path().string());

    if (!std::filesystem::exists(path()))
    {
        SPDLOG_LOGGER_INFO(
            SOKKETTER_LOGGER, "No device database file was found, skipping restoring.");
        return;
    }

    std::ifstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(SOKKETTER_LOGGER, "Failed restoring the device database!");
        return;
    }

    nlohmann::json j;
    file >> j;

    m_database = j.get<std::vector<std::unique_ptr<sokketter::power_strip>>>();
}

auto database_storage::path() const -> std::filesystem::path
{
    return sokketter::storage_path() / "devices.json";
}
