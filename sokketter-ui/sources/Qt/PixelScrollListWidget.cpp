#include "PixelScrollListWidget.h"

#include <QScrollBar>
#include <QWheelEvent>

PixelScrollListWidget::PixelScrollListWidget(QWidget *parent)
    : QListWidget(parent)
{}

void PixelScrollListWidget::wheelEvent(QWheelEvent *event)
{
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel)
    {
        const double speed_factor = 0.25;
        const int delta = static_cast<int>(event->angleDelta().y() * speed_factor);
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);
        event->accept();
    }
    else
    {
        QListWidget::wheelEvent(event);
    }
}
