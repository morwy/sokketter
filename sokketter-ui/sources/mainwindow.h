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

protected:
    auto closeEvent(QCloseEvent *event) -> void override;

private slots:
    auto onPowerStripClicked(QListWidgetItem *item) -> void;
    auto onSocketClicked(QListWidgetItem *item) -> void;

private:
    Ui::MainWindow *m_ui;
    std::shared_ptr<sokketter::power_strip> m_device = nullptr;

    auto repopulate_configure_list() -> void;
    auto redraw_configure_list() -> void;
    auto save_new_configuration() -> void;

    auto initialize_settings_page() -> void;
    auto initialize_about_page() -> void;

    auto connect_socket_list_on_click() -> void;

    auto event(QEvent *event) -> bool override;
    auto resizeEvent(QResizeEvent *event) -> void override;
    auto setThemeAccordingToMode() -> void;

    auto repopulate_device_list() -> void;
    auto repopulate_socket_list(const sokketter::power_strip_configuration &configuration) -> void;

    auto redraw_device_list() -> void;
    auto redraw_socket_list() -> void;
};

#endif // MAINWINDOW_H
