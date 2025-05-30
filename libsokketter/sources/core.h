#ifndef CORE_H
#define CORE_H

#pragma once

#include <libsokketter.h>
#include <spdlog/logger.h>

class core
{
public:
    core();
    ~core();

    auto settings() noexcept -> sokketter::settings_information;

    auto initialize_logger() -> void;
    auto deinitialize_logger() -> void;

private:
    sokketter::settings_information m_settings;
    const std::string m_logger_name = "libsokketter";
};

#endif // CORE_H
