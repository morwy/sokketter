#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#    include "macos_theme_change_detection.h"
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    /**
     * Force the X11 (xcb) backend so window decorations are drawn by the window
     * manager instead of Qt itself. GNOME/Mutter under native Wayland does not draw
     * server-side decorations for Wayland clients, unlike the AppImage's bundled Qt,
     * which only ships the xcb plugin and therefore always runs under XWayland with
     * native decorations. Respect an explicit override if the user set one.
     */
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM"))
    {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
#endif

    QApplication app(argc, argv);

#ifdef Q_OS_LINUX
    /**
     * Hardcode the default font. The AppImage's bundled Qt fails to resolve the
     * system's generic "sans-serif" alias (falls back to a bare "Sans Serif" 9pt),
     * while the .deb build's system Qt resolves it to "Ubuntu Sans" 11pt, so the
     * two packages render the same UI with different fonts. Requesting the
     * concrete family directly avoids depending on that alias resolution.
     */
    app.setFont(QFont("Ubuntu Sans", 11));
#endif

#ifdef Q_OS_MACOS
    registerForMacThemeChanges();
#endif

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
