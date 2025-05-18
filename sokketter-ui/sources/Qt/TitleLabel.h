#ifndef TITLE_LABEL_H
#define TITLE_LABEL_H

#pragma once

#include <QLabel>

class TitleLabel : public QLabel
{
    Q_OBJECT

public:
    TitleLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // TITLE_LABEL_H
