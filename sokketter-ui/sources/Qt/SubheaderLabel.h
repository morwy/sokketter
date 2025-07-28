#ifndef SUBHEADER_LABEL_H
#define SUBHEADER_LABEL_H

#pragma once

#include <QLabel>

class SubheaderLabel : public QLabel
{
    Q_OBJECT

public:
    SubheaderLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // SUBHEADER_LABEL_H
