/**
 * Based on the answer from the Qt Forum.
 * @author Gojir4 (https://forum.qt.io/user/gojir4)
 * @link
 * https://forum.qt.io/topic/101648/how-to-create-simply-virtual-led-indicator/2?_=1743619030242&lang=en-US
 */

#ifndef QLEDLABEL_H
#define QLEDLABEL_H

#include <QLabel>

class QLedLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QLedLabel(QWidget *parent = 0);

    enum State
    {
        StateOk,
        StateWarning,
        StateError
    };

signals:

public slots:
    void setState(State state);
    void setState(bool state);
};

#endif // QLEDLABEL_H
