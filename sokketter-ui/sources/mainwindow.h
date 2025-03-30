#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <libsokketter.h>

#include <QListWidget>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    auto on_power_strip_clicked(QListWidgetItem *item) -> void;
    auto on_socket_clicked(QListWidgetItem *item) -> void;

private:
    Ui::MainWindow *m_ui;

    auto repopulate_device_list() -> void;
    auto repopulate_socket_list(const sokketter::power_strip_configuration &configuration) -> void;
};

#endif // MAINWINDOW_H
