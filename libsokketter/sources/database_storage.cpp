#include <database_storage.h>

#include <devices/power_strip_base.h>
#include <devices/power_strip_factory.h>
#include <sokketter_core.h>

#include <fstream>
#include <json/json.hpp>
#include <spdlog/spdlog.h>

namespace sokketter {
    void to_json(nlohmann::json &j, const sokketter::socket_configuration &s)
    {
        j = nlohmann::json{{"id", s.id}, {"name", s.name}, {"description", s.description}};
    }

    void from_json(const nlohmann::json &j, sokketter::socket_configuration &s)
    {
        s.id = j.value("id", "");
        s.name = j.value("name", "");
        s.description = j.value("description", "");
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
        /**
         * @attention skip saving test devices.
         */
        if (ps.configuration().type == sokketter::power_strip_type::TEST_DEVICE)
        {
            return;
        }

        std::vector<sokketter::socket_configuration> sockets = {};
        for (const auto &socket : ps.sockets())
        {
            sockets.push_back(socket.configuration());
        }

        j = nlohmann::json{{"type", ps.configuration().type}, {"id", ps.configuration().id},
            {"name", ps.configuration().name}, {"description", ps.configuration().description},
            {"sockets", sockets}};
    }

    void from_json(const nlohmann::json &j, sokketter::power_strip &ps)
    {
        auto configuration = ps.configuration();

        configuration.type = j.value("type", sokketter::power_strip_type::UNKNOWN);
        configuration.id = j.value("id", "");
        configuration.name = j.value("name", "");
        configuration.description = j.value("description", "");

        ps.configure(configuration);

        const auto &socket_configurations =
            j.value("sockets", std::vector<sokketter::socket_configuration>());
        if (socket_configurations.empty())
        {
            SPDLOG_LOGGER_ERROR(
                SOKKETTER_LOGGER, "{}: Saved socket configuration is empty!", ps.to_string());
            return;
        }

        auto &sockets = ps.sockets();

        for (size_t index = 0; index < socket_configurations.size(); index++)
        {
            auto socket = sokketter::socket(socket_configurations[index]);
            sockets.push_back(socket);
        }
    }

    void to_json(nlohmann::json &j, const std::shared_ptr<sokketter::power_strip> &ptr)
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

    void from_json(const nlohmann::json &j, std::shared_ptr<sokketter::power_strip> &ptr)
    {
        if (j.is_null())
        {
            ptr = nullptr;
        }
        else
        {
            const auto &power_strip = j.get<sokketter::power_strip>();
            ptr = power_strip_factory::create(power_strip.configuration().type);
            if (auto *basePtr = dynamic_cast<power_strip_base *>(ptr.get()))
            {
                basePtr->copyFrom(power_strip);
            }
            else
            {
                SPDLOG_LOGGER_ERROR(
                    SOKKETTER_LOGGER, "Failed casting power_strip to power_strip_base.");
            }
        }
    }
} // namespace sokketter

auto database_storage::get() -> std::vector<std::shared_ptr<sokketter::power_strip>> &
{
    return m_devices;
}

auto database_storage::save() const -> void
{
    SPDLOG_LOGGER_DEBUG(
        SOKKETTER_LOGGER, "Saving the device database to '{}' file.", path().string());

    std::ofstream file(path().string());
    if (!file.is_open())
    {
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "Failed opening the device database file for writing!");
        return;
    }

    nlohmann::json j = m_devices;
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
        SPDLOG_LOGGER_ERROR(
            SOKKETTER_LOGGER, "Failed opening the device database file for reading!");
        return;
    }

    nlohmann::json j;
    file >> j;

    m_devices = j.get<std::vector<std::shared_ptr<sokketter::power_strip>>>();
}

auto database_storage::release_resources() -> void
{
    SPDLOG_LOGGER_DEBUG(SOKKETTER_LOGGER, "Releasing device database resources.");

    for (auto &device : m_devices)
    {
        device.reset();
        device = nullptr;
    }
}

auto database_storage::path() const -> std::filesystem::path
{
    return sokketter::storage_path() / "devices.json";
}
