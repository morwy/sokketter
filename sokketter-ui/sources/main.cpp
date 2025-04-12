#include "mainwindow.h"

#include "macos_theme_change_detection.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    registerForMacThemeChanges();

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
