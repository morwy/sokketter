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

void to_json(nlohmann::json &j, const window_settings &ws)
{
    j = nlohmann::json{{"x", ws.x}, {"y", ws.y}, {"width", ws.width}, {"height", ws.height}};
}

void from_json(const nlohmann::json &j, window_settings &ws)
{
    j.at("x").get_to(ws.x);
    j.at("y").get_to(ws.y);
    j.at("width").get_to(ws.width);
    j.at("height").get_to(ws.height);
}

struct app_settings
{
    window_settings window;
};

void to_json(nlohmann::json &j, const app_settings &s)
{
    j = nlohmann::json{{"window", s.window}};
}

void from_json(const nlohmann::json &j, app_settings &s)
{
    j.at("window").get_to(s.window);
}

#endif // APP_SETTINGS_H
