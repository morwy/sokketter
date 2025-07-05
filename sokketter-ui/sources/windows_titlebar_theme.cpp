#include "windows_titlebar_theme.h"

#ifdef Q_OS_WIN

#    include <app_logger.h>

#    include <Windows.h>
#    include <dwmapi.h>
#    include <spdlog/spdlog.h>

#    pragma comment(lib, "Dwmapi.lib")

void toggle_dark_titlebar(WId window_id, const bool enabled)
{
    HWND window_handle = reinterpret_cast<HWND>(window_id);
    BOOL use_dark_mode = enabled ? TRUE : FALSE;

    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;

    HRESULT result = DwmSetWindowAttribute(
        window_handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_mode, sizeof(use_dark_mode));
    if (FAILED(result))
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER,
            "DwmSetWindowAttribute() failed with error code {}, retrying with older version.",
            result);

        result = DwmSetWindowAttribute(window_handle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
            &use_dark_mode, sizeof(use_dark_mode));
        if (FAILED(result))
        {
            SPDLOG_LOGGER_ERROR(APP_LOGGER,
                "DwmSetWindowAttribute() with older version failed with error code {}.", result);
        }
    }

    SPDLOG_LOGGER_DEBUG(
        APP_LOGGER, "Set application title bar theme to {}.", enabled ? "dark" : "light");
}

#endif
