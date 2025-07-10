#include "SocketListItem.h"
#include "ui_SocketListItem.h"

#include <app_logger.h>
#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#include <QStyleHints>

SocketListItem::SocketListItem(const sokketter::power_strip_configuration &power_strip,
    const sokketter::socket_configuration &socket, const size_t &socket_index, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SocketListItem)
    , m_power_strip(power_strip)
    , m_socket(socket)
    , m_socket_index(socket_index)
{
    m_ui->setupUi(this);

    m_ui->status_label->setState(QLedLabel::State::StateUnknown);
    m_ui->status_label->setToolTip(tr("unknown"));

    const auto &title_text =
        "**Socket " + QString::number(socket_index) + "**: " + QString::fromStdString(socket.name);

    m_ui->name_label->setText(title_text);

    if (!socket.description.empty())
    {
        m_ui->description_label->setText(QString::fromStdString(socket.description));
    }
    else
    {
        m_ui->description_label->hide();
    }

    if (socket.configurable_reset_msec == 0)
    {
        m_ui->configurable_reset_button->hide();
    }
    else
    {
        m_ui->configurable_reset_button->show();

        const int msecs = m_socket.configurable_reset_msec;

        int precision = 0;
        if (msecs % 1000 != 0)
        {
            if (msecs % 100 == 0)
            {
                precision = 1;
            }
            else if (msecs % 10 == 0)
            {
                precision = 2;
            }
            else
            {
                precision = 3;
            }
        }

        const auto tooltip = QString::number(msecs / 1000, 'f', precision) + " seconds";
        m_ui->configurable_reset_button->setToolTip(tooltip);

        QObject::connect(m_ui->configurable_reset_button, &QPushButton::clicked, this,
            &SocketListItem::onResetButtonClicked);
    }

    setThemeAccordingToMode();
}

SocketListItem::~SocketListItem()
{
    delete m_ui;
}

auto SocketListItem::power_strip_configuration() const
    -> const sokketter::power_strip_configuration &
{
    return m_power_strip;
}

auto SocketListItem::socket_configuration() const -> const sokketter::socket_configuration &
{
    return m_socket;
}

auto SocketListItem::socket_index() const -> const size_t &
{
    return m_socket_index;
}

void SocketListItem::set_state(const bool is_on) const
{
    m_ui->status_label->setState(is_on);
    m_ui->status_label->setToolTip(is_on ? tr("powered on") : tr("powered off"));
}

void SocketListItem::toggle_reset_button_state(const bool is_enabled)
{
    m_ui->configurable_reset_button->setEnabled(is_enabled);
}

void SocketListItem::onResetButtonClicked(bool checked)
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Detected reset button click.");
    emit configurableResetRequested(this);
}

auto SocketListItem::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange)
    {
        SPDLOG_LOGGER_DEBUG(
            APP_LOGGER, "Detected mode change to {}.", isDarkMode() ? "dark" : "light");
        setThemeAccordingToMode();
        return true;
    }

    return QWidget::event(event);
}

void SocketListItem::setThemeAccordingToMode()
{
    QPixmap socket_pixmap;
    if (isDarkMode())
    {
        socket_pixmap.load(":/icons/socket_icon_white.png");
    }
    else
    {
        socket_pixmap.load(":/icons/socket_icon_black.png");
    }

    m_ui->icon_label->setPixmap(socket_pixmap);
}
