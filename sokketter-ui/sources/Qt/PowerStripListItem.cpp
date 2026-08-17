#include "PowerStripListItem.h"
#include "ui_PowerStripListItem.h"

#include <libsokketter.h>
#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#include <QPainter>
#include <QPixmap>
#include <QStyleHints>

PowerStripListItem::PowerStripListItem(
    const sokketter::power_strip_configuration &configuration, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::PowerStripListItem)
{
    m_ui->setupUi(this);

    configure(configuration);
    setThemeAccordingToMode();
}

PowerStripListItem::~PowerStripListItem()
{
    delete m_ui;
}

auto PowerStripListItem::configure(const sokketter::power_strip_configuration &configuration)
    -> void
{
    m_configuration = configuration;

    m_ui->name_label->setText(QString::fromStdString(configuration.name));

    auto type_n_address =
        QString::fromStdString(sokketter::power_strip_type_to_string(configuration.type));
    if (!configuration.address.empty())
    {
        type_n_address += ", available at " + QString::fromStdString(configuration.address);
    }

    m_ui->type_n_address_label->setText(type_n_address);

    if (!configuration.description.empty())
    {
        m_ui->description_label->show();
        m_ui->description_label->setText(QString::fromStdString(configuration.description));
    }
    else
    {
        m_ui->description_label->hide();
    }
}

auto PowerStripListItem::configuration() const -> const sokketter::power_strip_configuration &
{
    return m_configuration;
}

void PowerStripListItem::set_state(const bool is_on) const
{
    m_ui->status_label->setState(is_on);
    m_ui->status_label->setToolTip(is_on ? tr("connected") : tr("disconnected"));
}

auto PowerStripListItem::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange)
    {
        SPDLOG_DEBUG("Detected mode change to {}.", isDarkMode() ? "dark" : "light");
        setThemeAccordingToMode();
        return true;
    }

    return QWidget::event(event);
}

void PowerStripListItem::setThemeAccordingToMode()
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
