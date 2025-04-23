#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <empty_power_strip_list_item.h>
#include <power_strip_list_item.h>
#include <socket_list_item.h>
#include <theme_stylesheets.h>

#include <ClickableLabel.h>
#include <QApplication>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    /**
     * @brief apply the common stylesheet.
     */
    qApp->setStyleSheet(isDarkMode() ? dark_theme : light_theme);

    /**
     * @brief set the speed of the sliding stacked widget to 500 ms.
     */
    m_ui->stackedWidget->setSpeed(500);

    /**
     * @brief set the margins of clickable labels.
     */
    m_ui->power_strip_list_refresh_label->setContentsMargins(10, 0, 10, 0);
    m_ui->socket_list_back_label->setContentsMargins(10, 0, 10, 0);

    /**
     * @brief connect the signals to the slots.
     */
    QObject::connect(m_ui->power_strip_list_widget, &QListWidget::itemClicked, this,
        &MainWindow::onPowerStripClicked);

    QObject::connect(m_ui->power_strip_list_refresh_label, &ClickableLabel::clicked,
        [this]() { repopulate_device_list(); });

    QObject::connect(
        m_ui->socket_list_widget, &QListWidget::itemClicked, this, &MainWindow::onSocketClicked);

    QObject::connect(m_ui->socket_list_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->power_strip_list_page);
        m_ui->stackedWidget->slideInIdx(index);
        redraw_device_list();
    });

    QTimer::singleShot(25, [this]() { repopulate_device_list(); });
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

auto MainWindow::onPowerStripClicked(QListWidgetItem *item) -> void
{
    /**
     * @brief ignore click if it's a empty_power_strip_list_item.
     */
    QWidget *widget = m_ui->power_strip_list_widget->itemWidget(item);
    if (qobject_cast<empty_power_strip_list_item *>(widget))
    {
        return;
    }

    const int &index = m_ui->stackedWidget->indexOf(m_ui->socket_list_page);
    m_ui->stackedWidget->slideInIdx(index);

    const auto &configuration =
        dynamic_cast<power_strip_list_item *>(m_ui->power_strip_list_widget->itemWidget(item))
            ->configuration();

    repopulate_socket_list(configuration);
}

auto MainWindow::onSocketClicked(QListWidgetItem *item) -> void
{
    auto socket_item = dynamic_cast<socket_list_item *>(m_ui->socket_list_widget->itemWidget(item));
    if (socket_item == nullptr)
    {
        return;
    }

    const auto &power_strip_configuration = socket_item->power_strip_configuration();

    const auto &device = sokketter::device(power_strip_configuration.id);
    if (device == nullptr)
    {
        return;
    }

    const size_t &index = m_ui->socket_list_widget->row(item);

    const auto &socket_opt = device->socket(index);
    if (!socket_opt.has_value())
    {
        return;
    }

    const auto &socket = socket_opt->get();
    if (!socket.toggle())
    {
        return;
    }

    socket_item->set_state(socket.is_powered_on());
}

auto MainWindow::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange)
    {
        setThemeAccordingToMode();
        return true;
    }

    return QWidget::event(event);
}

void MainWindow::redraw_device_list()
{
    if (m_ui->power_strip_list_widget->isVisible())
    {
        int visibleWidth = m_ui->power_strip_list_widget->width();
        if (m_ui->power_strip_list_widget->verticalScrollBar()->isVisible())
        {
            visibleWidth -= m_ui->power_strip_list_widget->verticalScrollBar()->width();
        }

        int visibleHeight = m_ui->power_strip_list_widget->height();
        if (m_ui->power_strip_list_widget->horizontalScrollBar()->isVisible())
        {
            visibleHeight -= m_ui->power_strip_list_widget->horizontalScrollBar()->height();
        }

        const int &itemCount = m_ui->power_strip_list_widget->count();
        for (int i = 0; i < itemCount; ++i)
        {
            QListWidgetItem *item = m_ui->power_strip_list_widget->item(i);
            QWidget *itemWidget = m_ui->power_strip_list_widget->itemWidget(item);
            if (itemWidget)
            {
                itemWidget->setMaximumWidth(visibleWidth);
                const auto &size_hint = itemWidget->sizeHint();
                item->setSizeHint(
                    QSize(std::max(size_hint.width(), visibleWidth), size_hint.height()));
            }
        }
    }
}

void MainWindow::redraw_socket_list()
{
    if (m_ui->socket_list_widget->isVisible())
    {
        int visibleWidth = m_ui->socket_list_widget->width();
        if (m_ui->socket_list_widget->verticalScrollBar()->isVisible())
        {
            visibleWidth -= m_ui->socket_list_widget->verticalScrollBar()->width();
        }

        const int &itemCount = m_ui->socket_list_widget->count();
        for (int i = 0; i < itemCount; ++i)
        {
            QListWidgetItem *item = m_ui->socket_list_widget->item(i);
            QWidget *itemWidget = m_ui->socket_list_widget->itemWidget(item);
            if (itemWidget)
            {
                itemWidget->setMaximumWidth(visibleWidth);
                const auto &size_hint = itemWidget->sizeHint();
                item->setSizeHint(
                    QSize(std::max(size_hint.width(), visibleWidth), size_hint.height()));
            }
        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    redraw_device_list();
    redraw_socket_list();
}

void MainWindow::setThemeAccordingToMode()
{
    qApp->setStyleSheet(isDarkMode() ? dark_theme : light_theme);
}

auto MainWindow::repopulate_device_list() -> void
{
    while (m_ui->power_strip_list_widget->count() > 0)
    {
        m_ui->power_strip_list_widget->takeItem(0);
    }

    int visibleWidth = m_ui->power_strip_list_widget->width();
    if (m_ui->power_strip_list_widget->verticalScrollBar()->isVisible())
    {
        visibleWidth -= m_ui->power_strip_list_widget->verticalScrollBar()->width();
    }

    int visibleHeight = m_ui->power_strip_list_widget->height();
    if (m_ui->power_strip_list_widget->horizontalScrollBar()->isVisible())
    {
        visibleHeight -= m_ui->power_strip_list_widget->horizontalScrollBar()->height();
    }

    const auto &power_strips = sokketter::devices();
    for (const auto &power_strip : power_strips)
    {
        auto *power_strip_item = new power_strip_list_item(power_strip->configuration());
        power_strip_item->setMaximumWidth(visibleWidth);
        power_strip_item->set_state(power_strip->is_connected());

        auto *item = new QListWidgetItem();
        const auto &size_hint = power_strip_item->sizeHint();
        item->setSizeHint(QSize(std::max(size_hint.width(), visibleWidth), size_hint.height()));
        m_ui->power_strip_list_widget->addItem(item);
        m_ui->power_strip_list_widget->setItemWidget(item, power_strip_item);
    }

    if (power_strips.empty())
    {
        auto *empty_item = new empty_power_strip_list_item();
        empty_item->setMaximumHeight(visibleHeight);
        empty_item->setMaximumWidth(visibleWidth);

        auto *item = new QListWidgetItem();
        item->setSizeHint(QSize(visibleWidth, visibleHeight));
        m_ui->power_strip_list_widget->addItem(item);
        m_ui->power_strip_list_widget->setItemWidget(item, empty_item);
    }
}

auto MainWindow::repopulate_socket_list(const sokketter::power_strip_configuration &configuration)
    -> void
{
    while (m_ui->socket_list_widget->count() > 0)
    {
        m_ui->socket_list_widget->takeItem(0);
    }

    const auto &device = sokketter::device(configuration.id);
    if (device == nullptr)
    {
        return;
    }

    int visibleWidth = m_ui->socket_list_widget->width();
    if (m_ui->socket_list_widget->verticalScrollBar()->isVisible())
    {
        visibleWidth -= m_ui->socket_list_widget->verticalScrollBar()->width();
    }

    size_t socket_index = 1;

    const auto &sockets = device->sockets();
    for (const auto &socket : sockets)
    {
        auto *socket_item =
            new socket_list_item(configuration, socket.configuration(), socket_index);
        socket_item->setMaximumWidth(visibleWidth);
        socket_item->set_state(socket.is_powered_on());

        auto *item = new QListWidgetItem();
        const auto &size_hint = socket_item->sizeHint();
        item->setSizeHint(QSize(std::max(size_hint.width(), visibleWidth), size_hint.height()));
        m_ui->socket_list_widget->addItem(item);
        m_ui->socket_list_widget->setItemWidget(item, socket_item);

        socket_index++;
    }
}
