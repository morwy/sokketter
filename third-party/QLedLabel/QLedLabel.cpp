/**
 * Based on the answer from the Qt Forum.
 * @author Gojir4 (https://forum.qt.io/user/gojir4)
 * @link
 * https://forum.qt.io/topic/101648/how-to-create-simply-virtual-led-indicator/2?_=1743619030242&lang=en-US
 */

#include "QLedLabel.h"

#include <QDebug>

static const int SIZE = 8;
static const QString graySS =
    QString("color: white;border-radius: %1;background-color: rgba(111, 109, 102, 1.0);")
        .arg(SIZE / 2);
static const QString greenSS =
    QString("color: white;border-radius: %1;background-color: rgba(70, 167, 88, 1.0);")
        .arg(SIZE / 2);
static const QString orangeSS =
    QString("color: white;border-radius: %1;background-color: rgba(255, 197, 61, 1.0);")
        .arg(SIZE / 2);
static const QString redSS =
    QString("color: white;border-radius: %1;background-color: rgba(229, 72, 77, 1.0);")
        .arg(SIZE / 2);

QLedLabel::QLedLabel(QWidget *parent)
    : QLabel(parent)
{
    setState(StateUnknown);
    setFixedSize(SIZE, SIZE);
}

void QLedLabel::setState(State state)
{
    switch (state)
    {
    default:
    case StateUnknown:
        setStyleSheet(graySS);
        break;
    case StateOk:
        setStyleSheet(greenSS);
        break;
    case StateWarning:
        setStyleSheet(orangeSS);
        break;
    case StateError:
        setStyleSheet(redSS);
        break;
    }
}

void QLedLabel::setState(bool state)
{
    setState(state ? StateOk : StateError);
}
