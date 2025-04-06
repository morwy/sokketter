#include "socket_list_item.h"
#include "ui_socket_list_item.h"

socket_list_item::socket_list_item(const sokketter::power_strip_configuration &power_strip,
    const sokketter::socket_configuration &socket, const size_t &socket_index, QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::socket_list_item)
    , m_power_strip(power_strip)
    , m_socket(socket)
    , m_socket_index(socket_index)
{
    m_ui->setupUi(this);

    const auto &index_text = "Socket " + QString::number(socket_index);

    m_ui->index_label->setText(index_text);
    m_ui->name_label->setText(QString::fromStdString(socket.name));
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
    m_ui->status_label->setState(is_on);
    m_ui->status_label->setToolTip(is_on ? tr("powered on") : tr("powered off"));
}
