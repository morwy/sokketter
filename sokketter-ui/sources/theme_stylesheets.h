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
        border-radius: 8px;
    }

    QListWidget::item:selected {
        background: transparent;
        color: black;
    }

    QTextBrowser {
        outline: none;
        background: transparent;
    }
)";

const QString light_theme = base_theme + R"(
    QMainWindow {
        background-color: #FFFFFF;
    }

    QDialog {
        background-color: #FFFFFF;
    }

    QLabel {
        color: #21201C;
    }

    QFrame[frameShape="4"] {
        max-height: 1px;
        border: none;
        background-color: #F0F0EE;
    }

    QFrame[frameShape="5"] {
        max-height: 1px;
        border: none;
        background-color: #F0F0EE;
    }

    QListWidget::item:hover {
        background-color: #F1F0EF;
    }

    QListWidget::item:selected {
        background-color: #E9E8E6;
    }

    QTabBar::tab {
        color: #B06E0E;
        background: #FFF7C2;
    }

    QTabBar::tab:selected {
        color: #26241C;
        background: #FFC53D;
    }

    TitleLabel {
        color: #21201C;
        font-size: 16px;
        font-weight: bold;
    }

    ButtonLabel {
        color: #AB6400;
    }

    ButtonLabel:hover {
        color: #AB6400;
    }

    ButtonLabel:pressed {
        color: #AB6400;
    }

    QPushButton {
        background-color: #FFF7C2;
        color: #B06E0E;
        border: none;
        padding: 5px 10px;
        border-radius: 5px;
    }

    QPushButton:hover {
        background-color: #FFEDA4;
        color: #B06E0E;
        border: none;
        padding: 5px 10px;
        border-radius: 5px;
    }
)";

const QString dark_theme = base_theme + R"(
    QMainWindow {
        background-color: #191918;
    }

    QDialog {
        background-color: #191918;
    }

    QLabel {
        color: #EEEEEC;
    }

    QFrame[frameShape="4"] {
        max-height: 1px;
        border: none;
        background-color: #21211F;
    }

    QFrame[frameShape="5"] {
        max-height: 1px;
        border: none;
        background-color: #21211F;
    }

    QListWidget::item:hover {
        background-color: #222325;
    }

    QListWidget::item:selected {
        background-color: #292A2E;
    }

    QTabBar::tab {
        color: #FFCA16;
        background: #382715;
    }

    QTabBar::tab:selected {
        color: #26241C;
        background: #FFC53D;
    }

    TitleLabel {
        color: #EEEEEC;
        font-size: 16px;
        font-weight: bold;
    }

    ButtonLabel {
        color: #EDBC15;
    }

    ButtonLabel:hover {
        color: #EDBC15;
    }

    ButtonLabel:pressed {
        color: #EDBC15;
    }

    QPushButton {
        background-color: #382715;
        color: #FFCA16;
        border: none;
        padding: 5px 10px;
        border-radius: 5px;
    }

    QPushButton:hover {
        background-color: #462E17;
        color: #FFCA16;
        border: none;
        padding: 5px 10px;
        border-radius: 5px;
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
