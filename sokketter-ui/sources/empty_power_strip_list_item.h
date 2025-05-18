#ifndef EMPTY_POWER_STRIP_LIST_ITEM_H
#define EMPTY_POWER_STRIP_LIST_ITEM_H

#include <QWidget>

namespace Ui {
    class empty_power_strip_list_item;
}

class empty_power_strip_list_item : public QWidget
{
    Q_OBJECT

public:
    explicit empty_power_strip_list_item(QWidget *parent = nullptr);
    ~empty_power_strip_list_item();

private:
    Ui::empty_power_strip_list_item *ui;
};

#endif // EMPTY_POWER_STRIP_LIST_ITEM_H
