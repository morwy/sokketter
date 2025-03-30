#include "socket_list_item.h"
#include "ui_socket_list_item.h"

socket_list_item::socket_list_item(const sokketter::power_strip_configuration &power_strip,
    const sokketter::socket_configuration &socket, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::socket_list_item)
    , m_power_strip(power_strip)
    , m_socket(socket)
{
    m_ui->setupUi(this);

    m_ui->name_label->setText(QString::fromStdString(socket.name));
    m_ui->description_label->setText(QString::fromStdString(socket.id));
}

socket_list_item::~socket_list_item()
{
    delete m_ui;
}

auto socket_list_item::power_strip_configuration() const
    -> const sokketter::power_strip_configuration &
{
    return m_power_strip;
}

auto socket_list_item::socket_configuration() const -> const sokketter::socket_configuration &
{
    return m_socket;
}

void socket_list_item::set_state(const bool is_on) const
{
    m_ui->status_label->setText(is_on ? "on" : "off");
}
