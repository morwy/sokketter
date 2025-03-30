#include "power_strip_list_item.h"
#include "ui_power_strip_list_item.h"

#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

power_strip_list_item::power_strip_list_item(
    const sokketter::power_strip_configuration &configuration, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::power_strip_list_item)
    , m_configuration(configuration)
{
    m_ui->setupUi(this);

    m_ui->name_label->setText(QString::fromStdString(configuration.name));
    m_ui->address_label->setText(QString::fromStdString(configuration.address));
}

power_strip_list_item::~power_strip_list_item()
{
    delete m_ui;
}

auto power_strip_list_item::configuration() const -> const sokketter::power_strip_configuration &
{
    return m_configuration;
}
