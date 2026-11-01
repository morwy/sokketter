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

    m_ui->type_n_address_label->setText(
        QString::fromStdString(sokketter::power_strip_type_to_string(configuration.type)));

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
    /**
     * @brief the address text is always refreshed, independently of whether the connectivity
     * indicator had to be restyled, so that a stored last-known address is never shown for a
     * device that went offline.
     */
    update_type_n_address_label(is_on);

    if (m_state.has_value() && m_state.value() == is_on)
    {
        return;
    }

    m_state = is_on;

    m_ui->status_label->setState(is_on);
    m_ui->status_label->setToolTip(is_on ? tr("connected") : tr("disconnected"));
}

auto PowerStripListItem::update_type_n_address_label(bool is_on) const -> void
{
    auto type_n_address =
        QString::fromStdString(sokketter::power_strip_type_to_string(m_configuration.type));

    if (is_on && !m_configuration.address.empty())
    {
        type_n_address += ", available at " + QString::fromStdString(m_configuration.address);
    }

    m_ui->type_n_address_label->setText(type_n_address);
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
