#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#    include "macos_theme_change_detection.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef Q_OS_LINUX
    /**
     * Force a consistent Qt style so the UI looks identical regardless of the host's
     * platform theme plugin availability, which differs between the self-contained
     * AppImage and the .deb package linked against the system's Qt installation.
     */
    QApplication::setStyle(QStyleFactory::create("Fusion"));
#endif

#ifdef Q_OS_MACOS
    registerForMacThemeChanges();
#endif

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
