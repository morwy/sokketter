#include <app_settings.h>

void to_json(nlohmann::json &j, const window_settings &ws)
{
    j = nlohmann::json{{"x", ws.x}, {"y", ws.y}, {"width", ws.width}, {"height", ws.height}};
}

void from_json(const nlohmann::json &j, window_settings &ws)
{
    ws.x = j.value("x", 0);
    ws.y = j.value("y", 0);
    ws.width = j.value("width", 450);
    ws.height = j.value("height", 475);
}

void to_json(nlohmann::json &j, const app_settings &s)
{
    j = nlohmann::json{{"window", s.window}, {"socket_toggle", s.socket_toggle}};
}

void from_json(const nlohmann::json &j, app_settings &s)
{
    s.window = j.value("window", window_settings());
    s.socket_toggle = j.value("socket_toggle", socket_toggle_type::ST_SINGLE_CLICK);
}
