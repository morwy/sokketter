#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#pragma once

#include <filesystem>

constexpr auto LOGGER_NAME = "sokketter-ui";
#define APP_LOGGER spdlog::get(LOGGER_NAME)

auto initialize_app_logger(const std::filesystem::path &path) -> void;
auto deinitialize_app_logger() -> void;

#endif // APP_LOGGER_H
