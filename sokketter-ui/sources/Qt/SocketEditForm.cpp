#include "SocketEditForm.h"
#include "ui_SocketEditForm.h"

#include <QValidator>

SocketEditForm::SocketEditForm(
    const QString &title, const sokketter::socket_configuration &configuration, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::SocketEditForm)
{
    m_ui->setupUi(this);

    m_ui->socket_title_label->setText(title);

    m_ui->socket_name_line_edit->setText(QString::fromStdString(configuration.name));
    m_ui->socket_description_line_edit->setText(QString::fromStdString(configuration.description));

    m_ui->socket_configurable_reset_line_edit->setValidator(new QIntValidator(0, 999999999, this));
}

SocketEditForm::~SocketEditForm()
{
    delete m_ui;
}

auto SocketEditForm::configuration() const -> sokketter::socket_configuration
{
    auto configuration = sokketter::socket_configuration();

    configuration.name = m_ui->socket_name_line_edit->text().toStdString();
    configuration.description = m_ui->socket_description_line_edit->text().toStdString();
    configuration.configurable_reset_msec =
        m_ui->socket_configurable_reset_line_edit->text().toLongLong();

    return configuration;
}
