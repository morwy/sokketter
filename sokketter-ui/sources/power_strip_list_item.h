#ifndef POWER_STRIP_LIST_ITEM_H
#define POWER_STRIP_LIST_ITEM_H

#pragma once

#include <libsokketter.h>

#include <QWidget>

namespace Ui {
    class power_strip_list_item;
}

class power_strip_list_item : public QWidget
{
    Q_OBJECT

public:
    explicit power_strip_list_item(
        const sokketter::power_strip_configuration &configuration, QWidget *parent = nullptr);
    ~power_strip_list_item();

    auto configuration() const -> const sokketter::power_strip_configuration &;

    auto set_state(const bool is_on) const -> void;

private:
    Ui::power_strip_list_item *m_ui;
    const sokketter::power_strip_configuration m_configuration;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

#endif // POWER_STRIP_LIST_ITEM_H
