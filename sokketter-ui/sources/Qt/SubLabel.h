#ifndef SUB_LABEL_H
#define SUB_LABEL_H

#pragma once

#include <QLabel>

class SubLabel : public QLabel
{
    Q_OBJECT

public:
    SubLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // SUB_LABEL_H
