#ifndef THEME_STYLESHEETS_H
#define THEME_STYLESHEETS_H

#pragma once

#include <QApplication>
#include <QString>
#include <QStyleHints>

const QString base_theme = R"(
    QListWidget {
        outline: none;
        background: transparent;
    }

    QListWidget::item {
        border: none;
    }
)";

const QString light_theme = base_theme + R"(
    QMainWindow {
        background-color: #EDEEF0;
    }

    QLabel {
        color: #111113;
    }

    QListWidget::item:hover {
        background-color: #B0B4BA;
    }

    QListWidget::item:selected {
        background: transparent;
        color: black;
    }
)";

const QString dark_theme = base_theme + R"(
    QMainWindow {
        background-color: #18191B;
    }

    QLabel {
        color: #EDEEF0;
    }

    QListWidget::item:hover {
        background-color: #212225;
    }

    QListWidget::item:selected {
        background: transparent;
        color: black;
    }
)";

static auto isDarkMode() -> bool
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark;
#else
    const QPalette palette;
    const auto text = palette.color(QPalette::WindowText);
    const auto window = palette.color(QPalette::Window);
    return text.lightness() > window.lightness();
#endif
}

#endif // THEME_STYLESHEETS_H
