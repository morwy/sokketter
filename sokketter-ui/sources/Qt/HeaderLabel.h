#ifndef HEADER_LABEL_H
#define HEADER_LABEL_H

#pragma once

#include <QLabel>

class HeaderLabel : public QLabel
{
    Q_OBJECT

public:
    HeaderLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // HEADER_LABEL_H
