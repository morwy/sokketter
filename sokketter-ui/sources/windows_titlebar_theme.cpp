#include "windows_titlebar_theme.h"

#ifdef Q_OS_WIN

#    include <Windows.h>
#    include <dwmapi.h>

#    pragma comment(lib, "Dwmapi.lib")

void toggleDarkTitlebar(WId window_id, const bool enabled)
{
    HWND window_handle = reinterpret_cast<HWND>(window_id);
    BOOL useDarkMode = enabled ? TRUE : FALSE;

    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;

    HRESULT hr = DwmSetWindowAttribute(
        window_handle, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof(useDarkMode));
    if (FAILED(hr))
    {
        hr = DwmSetWindowAttribute(window_handle, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1,
            &useDarkMode, sizeof(useDarkMode));
    }
}

#endif
