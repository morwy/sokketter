#ifndef BUTTONLABEL_H
#define BUTTONLABEL_H

#pragma once

#include <ClickableLabel.h>

class ButtonLabel : public ClickableLabel
{
    Q_OBJECT

public:
    ButtonLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
};

#endif // BUTTONLABEL_H
