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

    TitleLabel {
        font-size: 16px;
        font-weight: bold;
    }

    Title2Label {
        font-size: 14px;
        font-weight: bold;
    }

    BoldLabel {
        font-weight: bold;
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

    QTextBrowser {
        border: none;
        color: #21201C;
        outline: none;
        background: #FFFFFF;
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

    HoverableListWidget::item:hover {
        background-color: #F1F0EF;
    }

    HoverableListWidget::item:selected {
        background-color: #E9E8E6;
    }

    QTabWidget {
        background: #FFFFFF;
    }

    QTabWidget::pane {
        border: 2px solid #FFC53D;
        background: #FFFFFF;
    }

    QWidget#tabPage {
        background-color: #FFFFFF;
    }

    QTabBar::tab {
        color: #B06E0E;
        background: #FFF7C2;
    }

    QTabBar::tab:selected {
        color: #26241C;
        background: #FFC53D;
    }

    QScrollBar:vertical {
        background: #FFF7C2;
    }

    QScrollBar::handle:vertical {
        background: #FFC53D;
    }

    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        background: none;
        height: 0px;
        border: none;
    }

    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical {
        background: none;
    }

    QScrollBar:horizontal {
        background: #FFF7C2;
    }

    QScrollBar::handle:horizontal {
        background: #FFC53D;
    }

    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal {
        background: none;
        height: 0px;
        border: none;
    }

    QScrollBar::add-page:horizontal,
    QScrollBar::sub-page:horizontal {
        background: none;
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

    QComboBox {
        outline: 0;
        background-color: #FFFFFF;
        color: #21201C;
        border: 1px solid #FFEDA4;
        padding: 3px;
    }

    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: top right;
        background-color: #FFFFFF;
        color: #21201C;
    }

    QComboBox QAbstractItemView {
        outline: 0;
        background-color: #FFFFFF;
        color: #21201C;
        selection-background-color: #FFC53D;
    }

    QComboBox QAbstractItemView::item:hover {
        background-color: #FFC53D;
        color: #26241C;
    }

    QComboBox QAbstractItemView::item:selected {
        background-color: #FFC53D;
        color: #26241C;
    }

    QLineEdit {
        border: 1px solid #CFCECA;
        border-radius: 5px;
        padding: 4px;
        background-color: #FFFFFF;
        color: #82827C;
    }

    QLineEdit:focus {
        border: 2px solid #E2A336;
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

    QTextBrowser {
        border: none;
        color: #EEEEEC;
        outline: none;
        background: #191918;
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

    HoverableListWidget::item:hover {
        background-color: #222325;
    }

    HoverableListWidget::item:selected {
        background-color: #292A2E;
    }

    QTabWidget {
        background: #191918;
    }

    QTabWidget::pane {
        border: 2px solid #FFC53D;
        background: #191918;
    }

    QWidget#tabPage {
        background-color: #191918;
    }

    QTabBar::tab {
        color: #FFCA16;
        background: #382715;
    }

    QTabBar::tab:selected {
        color: #26241C;
        background: #FFC53D;
    }

    QScrollBar:vertical {
        background: #382715;
    }

    QScrollBar::handle:vertical {
        background: #FFC53D;
    }

    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
        background: none;
        height: 0px;
        border: none;
    }

    QScrollBar::add-page:vertical,
    QScrollBar::sub-page:vertical {
        background: none;
    }

    QScrollBar:horizontal {
        background: #382715;
    }

    QScrollBar::handle:horizontal {
        background: #FFC53D;
    }

    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal {
        background: none;
        height: 0px;
        border: none;
    }

    QScrollBar::add-page:horizontal,
    QScrollBar::sub-page:horizontal {
        background: none;
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

    QComboBox {
        outline: 0;
        background-color: #191918;
        color: #EEEEEC;
        border: 1px solid #382715;
        padding: 3px;
    }

    QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: top right;
        background-color: #191918;
        color: #EEEEEC;
    }

    QComboBox QAbstractItemView {
        outline: 0;
        background-color: #191918;
        color: #EEEEEC;
        selection-background-color: #FFC53D;
    }

    QComboBox QAbstractItemView::item:hover {
        background-color: #FFC53D;
        color: #26241C;
    }

    QComboBox QAbstractItemView::item:selected {
        background-color: #FFC53D;
        color: #26241C;
    }

    QLineEdit {
        border: 1px solid #4F4E4A;
        border-radius: 5px;
        padding: 4px;
        background-color: #131312;
        color: #7d7c75;
    }

    QLineEdit:focus {
        border: 2px solid #8F6424;
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
