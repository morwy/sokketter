#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <SocketListItem.h>
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

signals:
    auto toggleResetButton(SocketListItem *object, bool is_on) -> void;

protected:
    auto closeEvent(QCloseEvent *event) -> void override;

private slots:
    auto onPowerStripClicked(QListWidgetItem *item) -> void;
    auto onSocketClicked(QListWidgetItem *item) -> void;
    auto onSocketResetClicked(SocketListItem *item) -> void;
    auto onResetButtonToggled(SocketListItem *item, bool is_on) -> void;

private:
    Ui::MainWindow *m_ui;
    std::shared_ptr<sokketter::power_strip> m_device = nullptr;

    auto repopulate_device_list() -> void;
    auto redraw_device_list() -> void;

    auto repopulate_socket_list() -> void;
    auto redraw_socket_list() -> void;

    auto repopulate_configure_list() -> void;
    auto redraw_configure_list() -> void;
    auto save_new_configuration() -> void;

    auto initialize_settings_page() -> void;
    auto initialize_about_page() -> void;

    auto connect_socket_list_on_click() -> void;

    auto event(QEvent *event) -> bool override;
    auto resizeEvent(QResizeEvent *event) -> void override;

    auto broadcast_event(QEvent *event) -> void;
    auto set_theme_according_to_mode() -> void;
};

#endif // MAINWINDOW_H
