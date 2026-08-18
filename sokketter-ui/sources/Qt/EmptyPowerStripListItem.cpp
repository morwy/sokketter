#include "EmptyPowerStripListItem.h"
#include "ui_EmptyPowerStripListItem.h"

EmptyPowerStripListItem::EmptyPowerStripListItem(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EmptyPowerStripListItem)
{
    ui->setupUi(this);
}

EmptyPowerStripListItem::~EmptyPowerStripListItem()
{
    delete ui;
}
