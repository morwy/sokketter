#ifndef UPDATE_CHECK_STORAGE_H
#define UPDATE_CHECK_STORAGE_H

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

/**
 * @brief persists the result of the latest background update check to disk.
 */
class update_check_storage
{
public:
    struct result
    {
        int64_t timestamp = 0;
        std::string new_version;
    };

    update_check_storage() = default;
    ~update_check_storage() = default;

    auto get() const -> result;
    auto set(const result &value) -> void;

    auto save() const -> void;
    auto load() -> void;

    auto path() const -> std::filesystem::path;

private:
    result m_result;
};

#endif // UPDATE_CHECK_STORAGE_H
