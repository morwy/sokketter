#include "ElidingLabel.h"

ElidingLabel::ElidingLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent)
{}

void ElidingLabel::setText(const QString &text)
{
    m_text = text;
    updateElidedText();
}

void ElidingLabel::resizeEvent(QResizeEvent *event)
{
    updateElidedText();
    QLabel::resizeEvent(event);
}

void ElidingLabel::updateElidedText()
{
    const QFontMetrics fontMetrics(font());
    const QString elidedText = fontMetrics.elidedText(m_text, Qt::ElideMiddle, width());
    QLabel::setText(elidedText);
}
