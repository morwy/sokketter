#ifndef HOVERABLELISTWIDGET_H
#define HOVERABLELISTWIDGET_H

#pragma once

#include <QListWidget>

class HoverableListWidget : public QListWidget
{
    Q_OBJECT

public:
    HoverableListWidget(QWidget *parent = nullptr);
};

#endif // HOVERABLELISTWIDGET_H
