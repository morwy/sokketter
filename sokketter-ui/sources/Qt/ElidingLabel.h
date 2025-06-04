#ifndef ELIDINGLABEL_H
#define ELIDINGLABEL_H

#pragma once

#include <QLabel>

class ElidingLabel : public QLabel
{
    Q_OBJECT

public:
    ElidingLabel(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

    void setText(const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QString m_text;

    void updateElidedText();
};

#endif // ELIDINGLABEL_H
