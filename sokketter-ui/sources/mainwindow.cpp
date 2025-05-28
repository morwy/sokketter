#include "mainwindow.h"
#include "license_dialog.h"
#include "ui_mainwindow.h"

#include <empty_power_strip_list_item.h>
#include <power_strip_list_item.h>
#include <socket_list_item.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#include <ClickableLabel.h>
#include <QApplication>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QTimer>

#ifdef Q_OS_WIN
#    include <windows_titlebar_theme.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    SPDLOG_INFO("Default logging.");

    m_ui->setupUi(this);

    setThemeAccordingToMode();

    /**
     * @brief set the speed of the sliding stacked widget to 500 ms.
     */
    m_ui->stackedWidget->setSpeed(500);

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

    QObject::connect(m_ui->power_strip_about_button, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->about_page);
        m_ui->stackedWidget->slideInIdx(index);
    });

    initialize_about_page();

    QTimer::singleShot(25, [this]() { repopulate_device_list(); });
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

auto MainWindow::initialize_about_page() -> void
{
    /**
     * @brief button signals.
     */
    QObject::connect(m_ui->about_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->power_strip_list_page);
        m_ui->stackedWidget->slideInIdx(index);
        redraw_device_list();
    });

    QObject::connect(m_ui->about_license_label, &ClickableLabel::clicked, [this]() {
        license_dialog *licenseDialog = new license_dialog(this);
        licenseDialog->show();
    });

    /**
     * @brief build information.
     */
    m_ui->about_version_label->setText(
        "sokketter version " + QString::fromStdString(sokketter::version().to_string()));
    m_ui->about_own_license_label->setText("Distributed under BCD 3-Clause license");
    m_ui->about_git_hash_label->setText(
        "Git commit hash: " + QString::fromStdString(sokketter::version().git_hash()));
    m_ui->about_build_date_label->setText(QString("Build on ") + __DATE__ + " at " + __TIME__);

    /**
     * @brief used components information.
     */
    const QString qtVersion = QString(QT_VERSION_STR);

    QString licenseInfoText = R"(
\- **Qt %QT_VERSION%:** used as dynamically linked libraries for providing re-linking mechanism and under acceptance of Accept Digital Rights Management terms. Qt souce code was not modified in any way and any possible modification will be non-proprietary and listed here in a detailed way. Distributed under LGPL v3 license ([link to license](https://www.gnu.org/licenses/lgpl-3.0.en.html#license-text)).

\- **radix-ui** as a color scheme constructor: made by WorkOS team ([link to profile on GitHub](https://github.com/workos)). Distributed under MIT license ([link to license](https://opensource.org/license/mit)).

\- **spdlog** library: made by Gabi Melman ([link to library on GitHub](https://github.com/gabime/spdlog)). Distributed under MIT license ([link to license](https://opensource.org/license/mit)).

\- "**Charge, charging, electric icon**" as an application icon: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578280/charge_charging_electric_electricity_plug_power_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, several parts outside of socket shape were removed.

\- "**Electric, electricity, energy icon**" as a socket picture in UI: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578127/electric_electricity_energy_plug_power_powerpoint_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, lines were made thinner and color was changed.

\- "**Electric, electricity, energy icon**" as a power strip picture in UI: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578128/electric_electricity_energy_plug_power_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, rotated, lines were made thinner and color was changed.
    )";

    licenseInfoText.replace("%QT_VERSION%", qtVersion);
    m_ui->used_components_text->setMarkdown(licenseInfoText);
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    redraw_device_list();
    redraw_socket_list();
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
        for (int index = 0; index < itemCount; ++index)
        {
            QListWidgetItem *item = m_ui->power_strip_list_widget->item(index);
            QWidget *widget = m_ui->power_strip_list_widget->itemWidget(item);
            if (widget == nullptr)
            {
                continue;
            }

            if (qobject_cast<empty_power_strip_list_item *>(widget))
            {
                widget->setMaximumWidth(visibleWidth);
                const auto &size_hint = widget->sizeHint();
                item->setSizeHint(QSize(std::max(size_hint.width(), visibleWidth),
                    std::max(size_hint.width(), visibleHeight)));
            }
            else if (qobject_cast<power_strip_list_item *>(widget))
            {
                widget->setMaximumWidth(visibleWidth);
                const auto &size_hint = widget->sizeHint();
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

void MainWindow::setThemeAccordingToMode()
{
    qApp->setStyleSheet(isDarkMode() ? dark_theme : light_theme);
#ifdef Q_OS_WIN
    toggleDarkTitlebar(winId(), isDarkMode());
#endif
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
