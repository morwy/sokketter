#ifndef SOCKETLISTITEM_H
#define SOCKETLISTITEM_H

#pragma once

#include <libsokketter.h>

#include <QDebug>
#include <QWidget>

namespace Ui {
    class SocketListItem;
}

class SocketListItem : public QWidget
{
    Q_OBJECT

public:
    explicit SocketListItem(const sokketter::power_strip_configuration &power_strip,
        const sokketter::socket_configuration &socket, const size_t &socket_index,
        QWidget *parent = nullptr);
    ~SocketListItem();

    auto power_strip_configuration() const -> const sokketter::power_strip_configuration &;
    auto socket_configuration() const -> const sokketter::socket_configuration &;
    auto socket_index() const -> const size_t &;

    auto set_state(const bool is_on) const -> void;

    auto toggle_reset_button_state(const bool is_enabled) -> void;

signals:
    auto configurableResetRequested(SocketListItem *object) -> void;

private slots:
    auto onResetButtonClicked(bool checked) -> void;

private:
    Ui::SocketListItem *m_ui;
    const sokketter::power_strip_configuration m_power_strip;
    const sokketter::socket_configuration m_socket;
    const size_t m_socket_index;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

Q_DECLARE_METATYPE(SocketListItem)

#endif // SOCKET_LIST_ITEM_H
