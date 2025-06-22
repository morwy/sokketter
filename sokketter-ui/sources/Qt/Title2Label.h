#ifndef TITLE2_LABEL_H
#define TITLE2_LABEL_H

#pragma once

#include <QLabel>

class Title2Label : public QLabel
{
    Q_OBJECT

public:
    Title2Label(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // TITLE_LABEL_H
