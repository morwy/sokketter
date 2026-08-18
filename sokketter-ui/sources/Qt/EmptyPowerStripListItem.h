#ifndef EMPTYPOWERSTRIPLISTITEM_H
#define EMPTYPOWERSTRIPLISTITEM_H

#pragma once

#include <QWidget>

namespace Ui {
    class EmptyPowerStripListItem;
}

class EmptyPowerStripListItem : public QWidget
{
    Q_OBJECT

public:
    explicit EmptyPowerStripListItem(QWidget *parent = nullptr);
    ~EmptyPowerStripListItem();

private:
    Ui::EmptyPowerStripListItem *ui;
};

#endif // EMPTYPOWERSTRIPLISTITEM_H
