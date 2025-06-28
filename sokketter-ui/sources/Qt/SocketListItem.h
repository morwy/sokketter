#ifndef SOCKETLISTITEM_H
#define SOCKETLISTITEM_H

#pragma once

#include <libsokketter.h>

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

    auto set_state(const bool is_on) const -> void;

private:
    Ui::SocketListItem *m_ui;
    const sokketter::power_strip_configuration m_power_strip;
    const sokketter::socket_configuration m_socket;
    const size_t m_socket_index;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

#endif // SOCKET_LIST_ITEM_H
