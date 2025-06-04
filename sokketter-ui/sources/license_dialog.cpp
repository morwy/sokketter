#include "license_dialog.h"
#include "ui_license_dialog.h"

#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#ifdef Q_OS_WIN
#    include <windows_titlebar_theme.h>
#endif

license_dialog::license_dialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::license_dialog)
{
    m_ui->setupUi(this);

    for (int widget_index = 0; widget_index < m_ui->tabWidget->count(); widget_index++)
    {
        m_ui->tabWidget->widget(widget_index)->setObjectName("tabPage");
    }

    setThemeAccordingToMode();
}

license_dialog::~license_dialog()
{
    delete m_ui;
}

auto license_dialog::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange)
    {
        SPDLOG_DEBUG("Detected mode change to {}.", isDarkMode() ? "dark" : "light");
        setThemeAccordingToMode();
        return true;
    }

    return QWidget::event(event);
}

auto license_dialog::setThemeAccordingToMode() -> void
{
    this->setStyleSheet(isDarkMode() ? dark_theme : light_theme);
#ifdef Q_OS_WIN
    toggle_dark_titlebar(winId(), isDarkMode());
#endif
}
