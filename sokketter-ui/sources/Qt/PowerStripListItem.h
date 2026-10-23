#ifndef POWER_STRIP_LIST_ITEM_H
#define POWER_STRIP_LIST_ITEM_H

#pragma once

#include <libsokketter.h>

#include <QWidget>

#include <optional>

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

    /**
     * @brief updates the connectivity indicator; re-applies the state only when it actually
     * changed, so that refreshing an unchanged item does not restyle the LED.
     */
    auto set_state(const bool is_on) const -> void;

private:
    Ui::PowerStripListItem *m_ui;
    sokketter::power_strip_configuration m_configuration;

    /**
     * @brief last connectivity state rendered by set_state; empty until the first call.
     */
    mutable std::optional<bool> m_state = std::nullopt;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

#endif // POWER_STRIP_LIST_ITEM_H
