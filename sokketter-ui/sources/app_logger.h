#ifndef APP_LOGGER_H
#define APP_LOGGER_H

#pragma once

#include <libsokketter.h>

constexpr auto LOGGER_NAME = "sokketter-ui";
#define APP_LOGGER spdlog::get(LOGGER_NAME)

auto initialize_app_logger() -> void;
auto deinitialize_app_logger() -> void;

auto logging_callback(const sokketter::callback_response_structure &response) -> void;

#endif // APP_LOGGER_H
