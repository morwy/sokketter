#include "mainwindow.h"

#ifdef __APPLE__
#    include "macos_theme_change_detection.h"
#endif

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef __APPLE__
    registerForMacThemeChanges();
#endif

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
