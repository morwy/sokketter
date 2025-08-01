#include "mainwindow.h"
#include "license_dialog.h"
#include "ui_mainwindow.h"

#include <Qt/DeviceEditForm.h>
#include <Qt/SocketEditForm.h>
#include <Qt/SocketListItem.h>
#include <app_logger.h>
#include <app_settings_storage.h>
#include <empty_power_strip_list_item.h>
#include <power_strip_list_item.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/spdlog.h>
#include <theme_stylesheets.h>

#ifdef Q_OS_WIN
#    include <windows_titlebar_theme.h>
#endif

#include <ClickableLabel.h>
#include <QApplication>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QEvent>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    initialize_app_logger();

    sokketter::initialize();

    m_ui->setupUi(this);

    app_settings_storage::instance().load();

    auto settings = app_settings_storage::instance().get();
    this->setGeometry(
        settings.window.x, settings.window.y, settings.window.width, settings.window.height);

    set_theme_according_to_mode();

    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "sokketter-ui has started.");

    /**
     * @brief connect the signals to the slots.
     */
    QObject::connect(this, &MainWindow::toggleResetButton, this, &MainWindow::onResetButtonToggled);

    QObject::connect(m_ui->power_strip_list_widget, &QListWidget::itemClicked, this,
        &MainWindow::onPowerStripClicked);

    connect_socket_list_on_click();

    QObject::connect(m_ui->power_strip_list_refresh_label, &ClickableLabel::clicked,
        [this]() { repopulate_device_list(); });

    QObject::connect(m_ui->power_strip_settings_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->settings_page);
        m_ui->stackedWidget->setCurrentIndex(index);
    });

    QObject::connect(m_ui->power_strip_about_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->about_page);
        m_ui->stackedWidget->setCurrentIndex(index);
    });

    QObject::connect(m_ui->socket_list_edit_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->device_configure_page);
        m_ui->stackedWidget->setCurrentIndex(index);

        repopulate_configure_list();
    });

    QObject::connect(m_ui->socket_list_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->power_strip_list_page);
        m_ui->stackedWidget->setCurrentIndex(index);

        redraw_device_list();
    });

    QObject::connect(m_ui->configure_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->socket_list_page);
        m_ui->stackedWidget->setCurrentIndex(index);
    });

    QObject::connect(m_ui->configure_save_label, &ClickableLabel::clicked, [this]() {
        save_new_configuration();

        const int &index = m_ui->stackedWidget->indexOf(m_ui->socket_list_page);
        m_ui->stackedWidget->setCurrentIndex(index);

        repopulate_device_list();
        repopulate_socket_list();
    });

    initialize_settings_page();
    initialize_about_page();

    QTimer::singleShot(25, [this]() { repopulate_device_list(); });
}

MainWindow::~MainWindow()
{
    if (m_device != nullptr)
    {
        m_device.reset();
        m_device = nullptr;
    }

    delete m_ui;

    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "sokketter-ui has finished.");

    sokketter::deinitialize();

    deinitialize_app_logger();
}

auto MainWindow::closeEvent(QCloseEvent *event) -> void
{
    auto &settings = app_settings_storage::instance().get();
    settings.window = {this->x(), this->y(), this->width(), this->height()};

    app_settings_storage::instance().save();

    QMainWindow::closeEvent(event);
}

auto MainWindow::repopulate_device_list() -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Repopulating power strip list.");

    while (m_ui->power_strip_list_widget->count() > 0)
    {
        m_ui->power_strip_list_widget->takeItem(0);
    }

    int visible_width = m_ui->power_strip_list_widget->width();
    if (m_ui->power_strip_list_widget->verticalScrollBar()->isVisible())
    {
        visible_width -= (m_ui->power_strip_list_widget->verticalScrollBar()->width() * 2);
    }

    int visible_height = m_ui->power_strip_list_widget->height();
    if (m_ui->power_strip_list_widget->horizontalScrollBar()->isVisible())
    {
        visible_height -= (m_ui->power_strip_list_widget->horizontalScrollBar()->height() * 2);
    }

    const auto &power_strips = sokketter::devices();
    for (const auto &power_strip : power_strips)
    {
        auto *power_strip_item = new power_strip_list_item(power_strip->configuration());
        power_strip_item->setMaximumWidth(visible_width);
        power_strip_item->set_state(power_strip->is_connected());

        auto *item = new QListWidgetItem();
        const auto &size_hint = power_strip_item->sizeHint();
        item->setSizeHint(QSize(std::max(size_hint.width(), visible_width), size_hint.height()));

        m_ui->power_strip_list_widget->addItem(item);
        m_ui->power_strip_list_widget->setItemWidget(item, power_strip_item);
    }

    if (power_strips.empty())
    {
        auto *empty_item = new empty_power_strip_list_item();
        empty_item->setMaximumHeight(visible_height);
        empty_item->setMaximumWidth(visible_width);

        auto *item = new QListWidgetItem();
        item->setSizeHint(QSize(visible_width, visible_height));

        m_ui->power_strip_list_widget->addItem(item);
        m_ui->power_strip_list_widget->setItemWidget(item, empty_item);
    }

    redraw_device_list();
}

auto MainWindow::redraw_device_list() -> void
{
    if (m_ui->power_strip_list_widget->isVisible())
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Re-drawing power strip list.");

        int visible_width = m_ui->power_strip_list_widget->width();
        if (m_ui->power_strip_list_widget->verticalScrollBar()->isVisible())
        {
            visible_width -= (m_ui->power_strip_list_widget->verticalScrollBar()->width() * 2);
        }

        int visible_height = m_ui->power_strip_list_widget->height();
        if (m_ui->power_strip_list_widget->horizontalScrollBar()->isVisible())
        {
            visible_height -= (m_ui->power_strip_list_widget->horizontalScrollBar()->height() * 2);
        }

        const int &item_count = m_ui->power_strip_list_widget->count();
        for (int item_index = 0; item_index < item_count; ++item_index)
        {
            auto *list_widget = m_ui->power_strip_list_widget->item(item_index);
            if (list_widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(
                    APP_LOGGER, "Failed getting list widget item from the device list!");
                continue;
            }

            auto *widget = m_ui->power_strip_list_widget->itemWidget(list_widget);
            if (widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(
                    APP_LOGGER, "Failed getting display widget item from the device list!");
                continue;
            }

            if (qobject_cast<empty_power_strip_list_item *>(widget))
            {
                widget->setMaximumWidth(visible_width);

                const auto &size_hint = widget->sizeHint();
                list_widget->setSizeHint(QSize(std::max(size_hint.width(), visible_width),
                    std::max(size_hint.width(), visible_height)));
            }
            else if (qobject_cast<power_strip_list_item *>(widget))
            {
                widget->setMaximumWidth(visible_width);

                const auto &size_hint = widget->sizeHint();
                list_widget->setSizeHint(
                    QSize(std::max(size_hint.width(), visible_width), size_hint.height()));
            }
        }
    }
}

auto MainWindow::repopulate_socket_list() -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Repopulating socket list.");

    while (m_ui->socket_list_widget->count() > 0)
    {
        m_ui->socket_list_widget->takeItem(0);
    }

    const auto &device_configuration = m_device->configuration();

    m_ui->socket_list_device_label->setText(
        "of " + QString::fromStdString(device_configuration.name));

    const auto &sockets = m_device->sockets();
    for (size_t socket_index = 0; socket_index < sockets.size(); socket_index++)
    {
        const auto &socket = sockets[socket_index];

        auto *socket_item =
            new SocketListItem(device_configuration, socket.configuration(), socket_index);
        socket_item->setEnabled(m_device->is_connected());
        if (m_device->is_connected())
        {
            socket_item->set_state(socket.is_powered_on());
            QObject::connect(socket_item, &SocketListItem::configurableResetRequested, this,
                &MainWindow::onSocketResetClicked);
        }

        const auto &size_hint = socket_item->sizeHint();

        auto *item = new QListWidgetItem();
        item->setSizeHint(QSize(size_hint.width(), size_hint.height()));

        m_ui->socket_list_widget->addItem(item);
        m_ui->socket_list_widget->setItemWidget(item, socket_item);
    }

    m_ui->socket_list_widget->setEnabled(m_device->is_connected());

    redraw_socket_list();
}

auto MainWindow::redraw_socket_list() -> void
{
    if (m_ui->socket_list_widget->isVisible())
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Re-drawing socket list.");

        int visible_width = m_ui->socket_list_widget->width();
        if (m_ui->socket_list_widget->verticalScrollBar()->isVisible())
        {
            visible_width -= (m_ui->socket_list_widget->verticalScrollBar()->width() * 2);
        }

        const int &item_count = m_ui->socket_list_widget->count();
        for (int item_index = 0; item_index < item_count; ++item_index)
        {
            auto *list_widget = m_ui->socket_list_widget->item(item_index);
            if (list_widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(
                    APP_LOGGER, "Failed getting list widget item from the socket list!");
                continue;
            }

            auto *widget = m_ui->socket_list_widget->itemWidget(list_widget);
            if (widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(
                    APP_LOGGER, "Failed getting display widget item from the socket list!");
                continue;
            }

            widget->setMaximumWidth(visible_width);

            const auto &size_hint = widget->sizeHint();
            list_widget->setSizeHint(
                QSize(std::max(size_hint.width(), visible_width), size_hint.height()));
        }
    }
}

auto MainWindow::repopulate_configure_list() -> void
{
    if (m_device == nullptr)
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "No currently saved device pointer is present!");
        return;
    }

    while (m_ui->configure_socket_list_widget->count() > 0)
    {
        m_ui->configure_socket_list_widget->takeItem(0);
    }

    /**
     * @brief populate device entry.
     */
    auto *form = new DeviceEditForm(m_device->configuration(), this);
    const auto &size_hint = form->sizeHint();

    auto *item = new QListWidgetItem();
    item->setSizeHint(QSize(size_hint.width(), size_hint.height()));

    m_ui->configure_socket_list_widget->addItem(item);
    m_ui->configure_socket_list_widget->setItemWidget(item, form);

    /**
     * @brief populate socket entries.
     */
    auto &sockets = m_device->sockets();
    for (size_t socket_index = 0; socket_index < sockets.size(); socket_index++)
    {
        const auto &socket = sockets[socket_index];

        const auto title = QString("Socket %1").arg(socket_index + 1);

        auto *form = new SocketEditForm(title, socket.configuration(), this);
        const auto &size_hint = form->sizeHint();

        auto *item = new QListWidgetItem();
        item->setSizeHint(QSize(size_hint.width(), size_hint.height()));

        m_ui->configure_socket_list_widget->addItem(item);
        m_ui->configure_socket_list_widget->setItemWidget(item, form);
    }

    redraw_configure_list();
}

auto MainWindow::redraw_configure_list() -> void
{
    if (m_ui->device_configure_page->isVisible())
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Re-drawing device configuration list.");

        int visible_width = m_ui->configure_socket_list_widget->width();
        if (m_ui->configure_socket_list_widget->verticalScrollBar()->isVisible())
        {
            visible_width -= (m_ui->configure_socket_list_widget->verticalScrollBar()->width() * 2);
        }

        const int &item_count = m_ui->configure_socket_list_widget->count();
        for (int item_index = 0; item_index < item_count; ++item_index)
        {
            auto *list_widget = m_ui->configure_socket_list_widget->item(item_index);
            if (list_widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(APP_LOGGER,
                    "Failed getting list widget item from the socket configuration form!");
                continue;
            }

            auto *widget = m_ui->configure_socket_list_widget->itemWidget(list_widget);
            if (widget == nullptr)
            {
                SPDLOG_LOGGER_ERROR(
                    APP_LOGGER, "Failed getting display widget item from the list widget!");
                continue;
            }

            widget->setMaximumWidth(visible_width);

            const auto &size_hint = widget->sizeHint();
            list_widget->setSizeHint(
                QSize(std::max(size_hint.width(), visible_width), size_hint.height()));
        }
    }
}

auto MainWindow::save_new_configuration() -> void
{
    if (m_device == nullptr)
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "No currently saved device pointer is present!");
        return;
    }

    auto &sockets = m_device->sockets();
    for (int item_index = 0; item_index < m_ui->configure_socket_list_widget->count(); item_index++)
    {
        auto *list_widget = m_ui->configure_socket_list_widget->item(item_index);
        if (list_widget == nullptr)
        {
            SPDLOG_LOGGER_ERROR(
                APP_LOGGER, "Failed getting list widget item from the socket configuration form!");
            continue;
        }

        auto *widget = m_ui->configure_socket_list_widget->itemWidget(list_widget);
        if (widget == nullptr)
        {
            SPDLOG_LOGGER_ERROR(
                APP_LOGGER, "Failed getting display widget item from the list widget!");
            continue;
        }

        if (qobject_cast<DeviceEditForm *>(widget) != nullptr)
        {
            auto configuration = m_device->configuration();

            const auto *form = qobject_cast<DeviceEditForm *>(
                m_ui->configure_socket_list_widget->itemWidget(list_widget));

            auto new_configuration = form->configuration();

            configuration.name = new_configuration.name;
            configuration.description = new_configuration.description;

            m_device->configure(configuration);
        }

        if (qobject_cast<SocketEditForm *>(widget) != nullptr)
        {
            const auto socket_index = item_index - 1;
            const auto *form = qobject_cast<SocketEditForm *>(
                m_ui->configure_socket_list_widget->itemWidget(list_widget));
            sockets[socket_index].configure(form->configuration());
        }
    }

    m_device->save();
}

auto MainWindow::initialize_settings_page() -> void
{
    auto &settings = app_settings_storage::instance().get();

    m_ui->settings_data_path_label->setText(
        QString::fromStdString(sokketter::storage_path().string()));
    m_ui->settings_data_path_label->setToolTip(
        QString::fromStdString(sokketter::storage_path().string()));

    QObject::connect(m_ui->settings_open_data_label, &ClickableLabel::clicked, [this]() {
        const QString &path = m_ui->settings_data_path_label->text();
        QFileInfo pathInfo(path);

        if (!pathInfo.exists())
        {
            SPDLOG_LOGGER_ERROR(
                APP_LOGGER, "Data storage path '{}' does not exist!", path.toStdString());
            return;
        }

        QDesktopServices::openUrl(
            QUrl(QUrl::fromLocalFile(pathInfo.absoluteFilePath()).toString()));
    });

    QButtonGroup *socket_toggle_button_group = new QButtonGroup(this);
    socket_toggle_button_group->addButton(m_ui->settings_socket_single_on_radio_button);
    socket_toggle_button_group->addButton(m_ui->settings_socket_double_on_radio_button);

    switch (settings.socket_toggle)
    {
    case socket_toggle_type::ST_SINGLE_CLICK: {
        m_ui->settings_socket_single_on_radio_button->setChecked(true);
        break;
    }
    case socket_toggle_type::ST_DOUBLE_CLICK: {
        m_ui->settings_socket_double_on_radio_button->setChecked(true);
        break;
    }
    case socket_toggle_type::ST_INVALID: {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Invalid socket toggle type provided!");
        break;
    }
    }

    QObject::connect(m_ui->settings_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->power_strip_list_page);
        m_ui->stackedWidget->setCurrentIndex(index);

        redraw_device_list();
    });

    auto socket_toggle_lambda = [&](bool checked) {
        if (m_ui->settings_socket_double_on_radio_button->isChecked())
        {
            settings.socket_toggle = socket_toggle_type::ST_DOUBLE_CLICK;
        }
        else
        {
            /**
             * Use the single click toggle as a default value in case of invalid selection.
             */
            settings.socket_toggle = socket_toggle_type::ST_SINGLE_CLICK;
        }

        app_settings_storage::instance().save();
        connect_socket_list_on_click();
    };

    QObject::connect(
        m_ui->settings_socket_single_on_radio_button, &QRadioButton::clicked, socket_toggle_lambda);
    QObject::connect(
        m_ui->settings_socket_double_on_radio_button, &QRadioButton::clicked, socket_toggle_lambda);

    QButtonGroup *theme_button_group = new QButtonGroup(this);
    theme_button_group->addButton(m_ui->settings_theme_auto_radio_button);
    theme_button_group->addButton(m_ui->settings_theme_light_radio_button);
    theme_button_group->addButton(m_ui->settings_theme_dark_radio_button);

    switch (settings.theme)
    {
    case theme_type::T_AUTO: {
        m_ui->settings_theme_auto_radio_button->setChecked(true);
        break;
    }
    case theme_type::T_LIGHT: {
        m_ui->settings_theme_light_radio_button->setChecked(true);
        break;
    }
    case theme_type::T_DARK: {
        m_ui->settings_theme_dark_radio_button->setChecked(true);
        break;
    }
    case theme_type::T_INVALID: {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Invalid theme type provided!");
        break;
    }
    }

    auto theme_lambda = [&](bool checked) {
        if (m_ui->settings_theme_auto_radio_button->isChecked())
        {
            settings.theme = theme_type::T_AUTO;
        }
        else if (m_ui->settings_theme_light_radio_button->isChecked())
        {
            settings.theme = theme_type::T_LIGHT;
        }
        else
        {
            /**
             * Use the dark as a default value in case of invalid selection.
             */
            settings.theme = theme_type::T_DARK;
        }

        app_settings_storage::instance().save();

        QEvent theme_change_event(QEvent::ThemeChange);
        broadcast_event(&theme_change_event);
    };

    QObject::connect(m_ui->settings_theme_auto_radio_button, &QRadioButton::clicked, theme_lambda);
    QObject::connect(m_ui->settings_theme_light_radio_button, &QRadioButton::clicked, theme_lambda);
    QObject::connect(m_ui->settings_theme_dark_radio_button, &QRadioButton::clicked, theme_lambda);
}

auto MainWindow::initialize_about_page() -> void
{
    /**
     * @brief button signals.
     */
    QObject::connect(m_ui->about_back_label, &ClickableLabel::clicked, [this]() {
        const int &index = m_ui->stackedWidget->indexOf(m_ui->power_strip_list_page);
        m_ui->stackedWidget->setCurrentIndex(index);

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
\- **Qt**, version %QT_VERSION%: used as dynamically linked libraries for providing re-linking mechanism and under acceptance of Accept Digital Rights Management terms. Qt souce code was not modified in any way and any possible modification will be non-proprietary and listed here in a detailed way. Distributed under LGPL v3 license ([link to license](https://www.gnu.org/licenses/lgpl-3.0.en.html#license-text)).

\- **radix-ui** as a color scheme constructor: made by WorkOS team ([link to the profile on GitHub](https://github.com/workos)). Distributed under MIT license ([link to license](https://opensource.org/license/mit)).

\- **spdlog** library, version 1.15.0: made by Gabi Melman ([link to the library on GitHub](https://github.com/gabime/spdlog)). Distributed under MIT license ([link to license](https://opensource.org/license/mit)).

\- **nlohmann/json** library, version 3.12.0: made by Niels Lohmann ([link to the library on GitHub](https://github.com/nlohmann/json)). Distributed under MIT license ([link to license](https://opensource.org/license/mit)).

\- "**Charge, charging, electric icon**" as an application icon: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578280/charge_charging_electric_electricity_plug_power_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, several parts outside of socket shape were removed.

\- "**Electric, electricity, energy icon**" as a socket picture in UI: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578127/electric_electricity_energy_plug_power_powerpoint_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, lines were made thinner and color was changed.

\- "**Electric, electricity, energy icon**" as a power strip picture in UI: made by Ümit Can Evleksiz ([link to profile on IconFinder](https://www.iconfinder.com/umitcan_07)), shared on IconFinder ([link to material on IconFinder](https://www.iconfinder.com/icons/2578128/electric_electricity_energy_plug_power_socket_icon)). Distributed under CC BY-NC 3.0 license ([link to license](https://creativecommons.org/licenses/by-nc/3.0/deed.en)). Icon was modified, rotated, lines were made thinner and color was changed.
    )";

    licenseInfoText.replace("%QT_VERSION%", qtVersion);
    m_ui->used_components_text->setMarkdown(licenseInfoText);
}

auto MainWindow::connect_socket_list_on_click() -> void
{
    QObject::disconnect(m_ui->socket_list_widget, &QListWidget::itemClicked, nullptr, nullptr);
    QObject::disconnect(
        m_ui->socket_list_widget, &QListWidget::itemDoubleClicked, nullptr, nullptr);

    auto &settings = app_settings_storage::instance().get();
    if (settings.socket_toggle == socket_toggle_type::ST_SINGLE_CLICK)
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Using single-click for socket activation.");
        QObject::connect(m_ui->socket_list_widget, &QListWidget::itemClicked, this,
            &MainWindow::onSocketClicked);
    }
    else if (settings.socket_toggle == socket_toggle_type::ST_DOUBLE_CLICK)
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Using double-click for socket activation.");
        QObject::connect(m_ui->socket_list_widget, &QListWidget::itemDoubleClicked, this,
            &MainWindow::onSocketClicked);
    }
    else
    {
        SPDLOG_LOGGER_WARN(APP_LOGGER,
            "Unsupported socket activation type provided {}, defaulting to single-click.",
            int(settings.socket_toggle));
        QObject::connect(m_ui->socket_list_widget, &QListWidget::itemClicked, this,
            &MainWindow::onSocketClicked);
    }
}

auto MainWindow::onPowerStripClicked(QListWidgetItem *item) -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Detected onPowerStripClicked() signal.");

    /**
     * @brief ignore click if it's a empty_power_strip_list_item.
     */
    QWidget *widget = m_ui->power_strip_list_widget->itemWidget(item);
    if (qobject_cast<empty_power_strip_list_item *>(widget))
    {
        SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Ignoring click on empty power strip item.");
        return;
    }

    const auto &configuration =
        dynamic_cast<power_strip_list_item *>(m_ui->power_strip_list_widget->itemWidget(item))
            ->configuration();

    if (m_device != nullptr)
    {
        m_device.reset();
        m_device = nullptr;
    }

    m_device = sokketter::device(configuration.id);
    if (m_device == nullptr)
    {
        return;
    }

    const int &index = m_ui->stackedWidget->indexOf(m_ui->socket_list_page);
    m_ui->stackedWidget->setCurrentIndex(index);

    repopulate_socket_list();
}

auto MainWindow::onSocketClicked(QListWidgetItem *item) -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Detected onSocketClicked() signal.");

    auto socket_item = dynamic_cast<SocketListItem *>(m_ui->socket_list_widget->itemWidget(item));
    if (socket_item == nullptr)
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed getting a socket from the UI list!");
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
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed getting a socket from device!");
        return;
    }

    const auto &socket = socket_opt->get();
    if (!socket.toggle())
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed toggling a socket!");
        return;
    }

    socket_item->set_state(socket.is_powered_on());
}

void MainWindow::onSocketResetClicked(SocketListItem *item)
{
    emit toggleResetButton(item, false);

    const auto &power_strip_configuration = item->power_strip_configuration();
    const auto &socket_configuration = item->socket_configuration();

    const auto &device = sokketter::device(power_strip_configuration.id);
    if (device == nullptr)
    {
        return;
    }

    const auto &socket_index = item->socket_index();
    const auto &socket_opt = device->socket(socket_index);
    if (!socket_opt.has_value())
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed getting a socket from device!");
        return;
    }

    const auto &socket = socket_opt->get();
    if (!socket.toggle())
    {
        SPDLOG_LOGGER_ERROR(APP_LOGGER, "Failed toggling a socket!");
        return;
    }

    socket.power(false);

    item->set_state(socket.is_powered_on());

    QTimer::singleShot(socket_configuration.configurable_reset_msec, [this, item, socket]() {
        socket.power(true);

        item->set_state(socket.is_powered_on());

        emit toggleResetButton(item, true);
    });
}

void MainWindow::onResetButtonToggled(SocketListItem *item, bool is_on)
{
    item->toggle_reset_button_state(is_on);
}

auto MainWindow::event(QEvent *event) -> bool
{
    if (event->type() == QEvent::ThemeChange)
    {
        SPDLOG_LOGGER_DEBUG(
            APP_LOGGER, "Detected mode change to {}.", isDarkMode() ? "dark" : "light");
        set_theme_according_to_mode();
        return true;
    }

    return QWidget::event(event);
}

auto MainWindow::resizeEvent(QResizeEvent *event) -> void
{
    SPDLOG_LOGGER_DEBUG(APP_LOGGER, "Detected resize event.");

    QMainWindow::resizeEvent(event);

    redraw_device_list();
    redraw_socket_list();
    redraw_configure_list();
}

void MainWindow::broadcast_event(QEvent *event)
{
    QCoreApplication::sendEvent(this, event);

    const auto top_level_widgets = QApplication::topLevelWidgets();
    for (auto *widget : top_level_widgets)
    {
        if (widget == nullptr)
        {
            continue;
        }

        QCoreApplication::sendEvent(widget, event);

        const auto children = widget->findChildren<QWidget *>();
        for (auto *child : children)
        {
            QCoreApplication::sendEvent(child, event);
        }
    }
}

auto MainWindow::set_theme_according_to_mode() -> void
{
    SPDLOG_LOGGER_DEBUG(
        APP_LOGGER, "Setting application theme to {}.", isDarkMode() ? "dark" : "light");

    qApp->setStyleSheet(isDarkMode() ? dark_theme : light_theme);
#ifdef Q_OS_WIN
    toggle_dark_titlebar(winId(), isDarkMode());
#endif
}
