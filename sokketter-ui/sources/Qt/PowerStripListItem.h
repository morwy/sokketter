#ifndef POWER_STRIP_LIST_ITEM_H
#define POWER_STRIP_LIST_ITEM_H

#pragma once

#include <libsokketter.h>

#include <QWidget>

namespace Ui {
    class PowerStripListItem;
}

class PowerStripListItem : public QWidget
{
    Q_OBJECT

public:
    explicit PowerStripListItem(
        const sokketter::power_strip_configuration &configuration, QWidget *parent = nullptr);
    ~PowerStripListItem();

    auto configure(const sokketter::power_strip_configuration &configuration) -> void;
    auto configuration() const -> const sokketter::power_strip_configuration &;

    auto set_state(const bool is_on) const -> void;

private:
    Ui::PowerStripListItem *m_ui;
    sokketter::power_strip_configuration m_configuration;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

#endif // PowerStripListItem_H
