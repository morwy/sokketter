/**
 * Based on the answer from the Qt Forum.
 * @author Gojir4 (https://forum.qt.io/user/gojir4)
 * @link https://forum.qt.io/topic/101648/how-to-create-simply-virtual-led-indicator/2?_=1743619030242&lang=en-US
 */

#include "qledlabel.h"

#include <QDebug>

static const int SIZE = 8;
static const QString greenSS = QString(
    "color: white;border-radius: %1;background-color: rgba(45, 190, 71, 1.0);")
                                   .arg(SIZE / 2);
static const QString redSS =
    QString("color: white;border-radius: %1;background-color: rgba(254, 81, 81, 1.0);")
        .arg(SIZE / 2);
static const QString orangeSS = QString(
    "color: white;border-radius: %1;background-color: rgba(254, 177, 64, 1.0);")
                                    .arg(SIZE / 2);

QLedLabel::QLedLabel(QWidget *parent)
    : QLabel(parent)
{
    // Set to ok by default
    setState(StateOk);
    setFixedSize(SIZE, SIZE);
}

void QLedLabel::setState(State state)
{
    qDebug() << "setState" << state;
    switch (state)
    {
    default:
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
