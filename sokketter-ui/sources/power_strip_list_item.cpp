#include "power_strip_list_item.h"
#include "ui_power_strip_list_item.h"

#include <libsokketter.h>
#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#include <QPainter>
#include <QPixmap>
#include <QStyleHints>

power_strip_list_item::power_strip_list_item(
    const sokketter::power_strip_configuration &configuration, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::power_strip_list_item)
    , m_configuration(configuration)
{
    m_ui->setupUi(this);

    m_ui->name_label->setText(QString::fromStdString(configuration.name));

    const auto &description =
        QString::fromStdString(sokketter::power_strip_type_to_string(configuration.type)) +
        " located at " + QString::fromStdString(configuration.address);
    m_ui->description_label->setText(description);

    setThemeAccordingToMode();
}

power_strip_list_item::~power_strip_list_item()
{
    delete m_ui;
}

auto power_strip_list_item::configuration() const -> const sokketter::power_strip_configuration &
{
    return m_configuration;
}

void power_strip_list_item::set_state(const bool is_on) const
{
    m_ui->status_label->setState(is_on);
    m_ui->status_label->setToolTip(is_on ? tr("connected") : tr("disconnected"));
}

auto power_strip_list_item::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange)
    {
        SPDLOG_DEBUG("Detected mode change to {}.", isDarkMode() ? "dark" : "light");
        setThemeAccordingToMode();
        return true;
    }

    return QWidget::event(event);
}

void power_strip_list_item::setThemeAccordingToMode()
{
    QPixmap pixmap;

    if (isDarkMode())
    {
        pixmap.load(":/icons/power_strip_icon_white.png");
    }
    else
    {
        pixmap.load(":/icons/power_strip_icon_black.png");
    }

    m_ui->icon_label->setPixmap(pixmap);
}
