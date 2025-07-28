#include "DeviceEditForm.h"
#include "ui_DeviceEditForm.h"

DeviceEditForm::DeviceEditForm(
    const sokketter::power_strip_configuration &configuration, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::DeviceEditForm)
{
    m_ui->setupUi(this);

    m_ui->device_name_line_edit->setText(QString::fromStdString(configuration.name));
    m_ui->device_description_line_edit->setText(QString::fromStdString(configuration.description));
}

DeviceEditForm::~DeviceEditForm()
{
    delete m_ui;
}

auto DeviceEditForm::configuration() const -> sokketter::power_strip_configuration
{
    auto configuration = sokketter::power_strip_configuration();

    configuration.name = m_ui->device_name_line_edit->text().toStdString();
    configuration.description = m_ui->device_description_line_edit->text().toStdString();

    return configuration;
}
