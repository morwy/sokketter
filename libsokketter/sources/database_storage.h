#ifndef DATABASE_STORAGE_H
#define DATABASE_STORAGE_H

#pragma once

#include <libsokketter.h>

#include <filesystem>

class database_storage
{
public:
    database_storage() = default;
    ~database_storage() = default;

    auto get() -> std::vector<std::shared_ptr<sokketter::power_strip>> &;

    auto save() const -> void;
    auto load() -> void;

    auto release_resources() -> void;

    auto path() const -> std::filesystem::path;

private:
    std::vector<std::shared_ptr<sokketter::power_strip>> m_devices;
};

#endif // DATABASE_STORAGE_H
