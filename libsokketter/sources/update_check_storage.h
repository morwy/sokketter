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
    update_check_storage() = default;
    ~update_check_storage() = default;

    auto get() const -> sokketter::update_check_status;
    auto set(const sokketter::update_check_status &value) -> void;

    auto save() const -> void;
    auto load() -> void;

    auto path() const -> std::filesystem::path;

private:
    sokketter::update_check_status m_result;
};

#endif // UPDATE_CHECK_STORAGE_H
