#ifndef BOLD_LABEL_H
#define BOLD_LABEL_H

#pragma once

#include <QLabel>

class BoldLabel : public QLabel
{
    Q_OBJECT

public:
    BoldLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // BOLD_LABEL_H
