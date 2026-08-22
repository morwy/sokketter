#ifndef UPDATE_CHECK_STORAGE_H
#define UPDATE_CHECK_STORAGE_H

#pragma once

#include "../include/libsokketter.h"

#include <filesystem>

/**
 * @brief persists the result of the latest background update check to disk.
 */
class update_check_storage
{
public:
    struct cached_status
    {
        sokketter::update_check_status status;
        std::string latest_version;
    };

    update_check_storage() = default;
    ~update_check_storage() = default;

    auto get() const -> cached_status;
    auto set(const cached_status &value) -> void;

    auto save() const -> void;
    auto load() -> void;

    auto path() const -> std::filesystem::path;

private:
    cached_status m_result;
};

#endif // UPDATE_CHECK_STORAGE_H
