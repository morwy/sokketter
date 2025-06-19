#ifndef DATABASE_STORAGE_H
#define DATABASE_STORAGE_H

#pragma once

#include <libsokketter.h>

#include <filesystem>

class database_storage
{
public:
    static auto instance() -> database_storage &
    {
        static database_storage instance;
        return instance;
    }

    database_storage(const database_storage &) = delete;
    auto operator=(const database_storage &) -> void = delete;

    auto get() -> std::vector<std::shared_ptr<sokketter::power_strip>> &;

    auto save() const -> void;
    auto load() -> void;

    auto path() const -> std::filesystem::path;

private:
    std::vector<std::shared_ptr<sokketter::power_strip>> m_database;

    database_storage() = default;
    ~database_storage() = default;
};

#endif // DATABASE_STORAGE_H
