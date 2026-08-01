#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#pragma once

#include <SocketListItem.h>
#include <libsokketter.h>

#include <QListWidget>
#include <QMainWindow>
#include <QThreadPool>

#include <functional>

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
    auto newPowerStripReceived(std::vector<std::shared_ptr<sokketter::power_strip>> power_strips)
        -> void;
    auto newStatusReceived(sokketter::enumeration_status status) -> void;
    auto toggleResetButton(SocketListItem *object, bool is_on) -> void;

protected:
    auto closeEvent(QCloseEvent *event) -> void override;

private slots:
    auto onNewPowerStripReceived(std::vector<std::shared_ptr<sokketter::power_strip>> power_strips)
        -> void;
    auto onNewStatusReceived(sokketter::enumeration_status status) -> void;
    auto onPowerStripClicked(QListWidgetItem *item) -> void;
    auto onSocketClicked(QListWidgetItem *item) -> void;
    auto onSocketResetClicked(SocketListItem *item) -> void;
    auto onResetButtonToggled(SocketListItem *item, bool is_on) -> void;

private:
    Ui::MainWindow *m_ui;
    std::shared_ptr<sokketter::power_strip> m_device = nullptr;

    /**
     * @brief serializes blocking device I/O onto a single worker thread so the UI stays responsive.
     */
    QThreadPool m_device_pool;

    auto new_devices_received(std::vector<std::shared_ptr<sokketter::power_strip>> power_strips)
        -> void;
    auto new_status_received(sokketter::enumeration_status status) -> void;

    /**
     * @brief runs a blocking device operation on the worker thread and delivers the resulting
     * socket state back on the UI thread.
     */
    auto run_device_task(std::function<bool()> work, std::function<void(bool)> on_done) -> void;

    /**
     * @brief reads all socket states of the current device in the background and updates the list.
     */
    auto refresh_socket_states_async() -> void;

    auto repopulate_device_list() -> void;
    auto redraw_device_list() -> void;

    auto repopulate_socket_list() -> void;
    auto redraw_socket_list() -> void;

    auto repopulate_configure_list() -> void;
    auto redraw_configure_list() -> void;
    auto save_new_configuration() -> void;

    auto populate_authentication_page() -> void;
    auto initialize_settings_page() -> void;
    auto initialize_about_page() -> void;

    auto connect_socket_list_on_click() -> void;

    auto event(QEvent *event) -> bool override;
    auto resizeEvent(QResizeEvent *event) -> void override;

    auto broadcast_event(QEvent *event) -> void;
    auto set_theme_according_to_mode() -> void;
};

#endif // MAINWINDOW_H
