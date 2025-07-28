#ifndef HEADER2_LABEL_H
#define HEADER2_LABEL_H

#pragma once

#include <QLabel>

class Header2Label : public QLabel
{
    Q_OBJECT

public:
    Header2Label(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // HEADER2_LABEL_H
