#include "empty_power_strip_list_item.h"
#include "ui_empty_power_strip_list_item.h"

empty_power_strip_list_item::empty_power_strip_list_item(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::empty_power_strip_list_item)
{
    ui->setupUi(this);
}

empty_power_strip_list_item::~empty_power_strip_list_item()
{
    delete ui;
}
