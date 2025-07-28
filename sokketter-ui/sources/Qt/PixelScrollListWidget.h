#ifndef PIXEL_SCROLL_LIST_WIDGET_H
#define PIXEL_SCROLL_LIST_WIDGET_H

#include <QListWidget>

class PixelScrollListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit PixelScrollListWidget(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
};

#endif // PIXEL_SCROLL_LIST_WIDGET_H
