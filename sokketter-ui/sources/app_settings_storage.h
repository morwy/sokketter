#ifndef APP_SETTINGS_STORAGE_H
#define APP_SETTINGS_STORAGE_H

#pragma once

#include <app_settings.h>

class app_settings_storage
{
public:
    static auto instance() -> app_settings_storage &
    {
        static app_settings_storage instance;
        return instance;
    }

    app_settings_storage(const app_settings_storage &) = delete;
    auto operator=(const app_settings_storage &) -> void = delete;

    auto get() -> app_settings &;
    auto set(const app_settings &settings) -> void;

    auto save() const -> void;
    auto load() -> void;

    auto path() const -> std::filesystem::path;

private:
    app_settings m_settings;

    app_settings_storage() = default;
    ~app_settings_storage() = default;
};

#endif // APP_SETTINGS_STORAGE_H
