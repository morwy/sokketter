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

private:
    sokketter::settings_structure m_settings;
    std::shared_ptr<spdlog::logger> m_logger = nullptr;
    database_storage m_database;

    sokketter_core() = default;
    ~sokketter_core() = default;

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;

    auto logging_callback(const kommpot::callback_response_structure &response) -> void;
};

#endif // CORE_H
