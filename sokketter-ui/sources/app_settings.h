#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#pragma once

#include <json/json.hpp>

struct window_settings
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

void to_json(nlohmann::json &j, const window_settings &ws);

void from_json(const nlohmann::json &j, window_settings &ws);

enum class socket_toggle_type
{
    ST_INVALID = -1,
    ST_SINGLE_CLICK,
    ST_DOUBLE_CLICK,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    socket_toggle_type, {
                            {socket_toggle_type::ST_INVALID, nullptr},
                            {socket_toggle_type::ST_SINGLE_CLICK, "single-click"},
                            {socket_toggle_type::ST_DOUBLE_CLICK, "double-click"},
                        })

enum class theme_type
{
    T_INVALID = -1,
    T_AUTO,
    T_LIGHT,
    T_DARK,
};

NLOHMANN_JSON_SERIALIZE_ENUM(theme_type, {
                                             {theme_type::T_INVALID, nullptr},
                                             {theme_type::T_AUTO, "auto"},
                                             {theme_type::T_LIGHT, "light"},
                                             {theme_type::T_DARK, "dark"},
                                         })

struct app_settings
{
    window_settings window;
    socket_toggle_type socket_toggle = socket_toggle_type::ST_SINGLE_CLICK;
    theme_type theme = theme_type::T_AUTO;
};

void to_json(nlohmann::json &j, const app_settings &s);

void from_json(const nlohmann::json &j, app_settings &s);

#endif // APP_SETTINGS_H
