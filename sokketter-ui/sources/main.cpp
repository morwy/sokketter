#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QFontInfo>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#    include "macos_theme_change_detection.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    /**
     * Prefer the X11 (xcb) backend so window decorations are drawn by the window
     * manager instead of Qt itself. GNOME/Mutter under native Wayland does not draw
     * server-side decorations for Wayland clients, unlike the AppImage's bundled Qt,
     * which only ships the xcb plugin and therefore always runs under XWayland with
     * native decorations. Only do this when an X display is actually reachable
     * (DISPLAY is set by X11 sessions and by XWayland); in Wayland-only sessions
     * without XWayland let Qt pick the native platform so the app can still start.
     * Respect an explicit override if the user set one.
     */
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM") && qEnvironmentVariableIsSet("DISPLAY"))
    {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif

    QApplication app(argc, argv);

#ifdef Q_OS_LINUX
    /**
     * Normalize the default font. The AppImage's bundled Qt does not resolve the
     * generic "sans-serif" alias through the platform theme (falls back to a bare
     * "Sans Serif" 9pt), while the .deb build's system Qt resolves it (e.g. to
     * "Ubuntu Sans" 11pt), so the two packages render the same UI with different
     * fonts. Resolve the alias explicitly via font matching (fontconfig), which
     * yields the distribution's actual default sans-serif family on any system.
     */
    const QString sans_family = QFontInfo(QFont(QStringLiteral("sans-serif"))).family();
    app.setFont(QFont(sans_family, 11));
#endif

#ifdef Q_OS_MACOS
    registerForMacThemeChanges();
#endif

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
