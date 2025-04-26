#include "mainwindow.h"

#include <QApplication>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#    include "macos_theme_change_detection.h"
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef Q_OS_MACOS
    registerForMacThemeChanges();
#endif

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
