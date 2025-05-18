#include "ButtonLabel.h"

#include <QEvent>

ButtonLabel::ButtonLabel(QWidget *parent, Qt::WindowFlags f)
    : ClickableLabel(parent)
{
    this->setMouseTracking(true);
    this->setAttribute(Qt::WidgetAttribute::WA_Hover, true);
}

bool ButtonLabel::event(QEvent *event)
{
    if (event->type() == QEvent::Enter)
    {
        QFont f = font();
        f.setUnderline(true);
        setFont(f);
    }
    else if (event->type() == QEvent::Leave)
    {
        QFont f = font();
        f.setUnderline(false);
        setFont(f);
    }

    return ClickableLabel::event(event);
}
