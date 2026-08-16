#ifndef SOKKETTER_CORE_H
#define SOKKETTER_CORE_H

#pragma once

#include <database_storage.h>
#include <libsokketter.h>
#include <spdlog/logger.h>
#include <third-party/kommpot/libkommpot/include/libkommpot.h>

constexpr auto LOGGER_NAME = "sokketter";
#define SOKKETTER_LOGGER spdlog::get(LOGGER_NAME)

class sokketter_core
{
public:
    static auto instance() -> sokketter_core &
    {
        static sokketter_core instance;
        return instance;
    }

    sokketter_core(const sokketter_core &) = delete;
    auto operator=(const sokketter_core &) -> void = delete;

    auto initialize() -> bool;
    auto deinitialize() -> bool;

    auto settings() noexcept -> sokketter::settings_structure;
    auto set_settings(const sokketter::settings_structure &settings) noexcept -> void;

    auto database() -> database_storage &;

    auto devices(const sokketter::device_filter &filter = {})
        -> const std::vector<std::shared_ptr<sokketter::power_strip>> &;

    auto devices(const sokketter::device_filter &filter, sokketter::device_callback device_cb,
        sokketter::status_callback status_cb) -> void;

    auto device(const size_t &index) -> std::shared_ptr<sokketter::power_strip>;

    auto device(const std::string &serial_number) -> std::shared_ptr<sokketter::power_strip>;

    auto release_link() -> std::string;
    auto is_new_release_available(std::string &latest_version) -> bool;

private:
    inline static constexpr auto RELEASE_LINK =
        "https://github.com/morwy/sokketter/releases/latest";
    inline static constexpr auto RELEASE_API_LINK =
        "https://api.github.com/repos/morwy/sokketter/releases/latest";

    sokketter::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger = nullptr;
    database_storage m_database;
    sokketter::device_callback m_device_cb = nullptr;
    sokketter::status_callback m_status_cb = nullptr;

    sokketter_core() = default;
    ~sokketter_core() = default;

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;

    struct curl_string_buffer
    {
        std::string data;
    };

    static auto write_response_data(char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t;
    static auto normalize_version_string(std::string version) -> std::string;
    static auto parse_version_parts(const std::string &version) -> std::vector<uint32_t>;

    auto is_newer_version(const std::string &current_version, const std::string &candidate_version)
        -> bool;

    auto logging_callback(const kommpot::callback_response_structure &response) -> void;

    auto new_devices_received(
        std::vector<std::shared_ptr<kommpot::device_communication>> communications) -> void;
    auto new_status_received(kommpot::enumeration_status status) -> void;
};

#endif // CORE_H
