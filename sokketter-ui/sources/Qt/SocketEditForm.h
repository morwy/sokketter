#ifndef SOCKETEDITFORM_H
#define SOCKETEDITFORM_H

#pragma once

#include <libsokketter.h>

#include <QWidget>

namespace Ui {
    class SocketEditForm;
}

class SocketEditForm : public QWidget
{
    Q_OBJECT

public:
    explicit SocketEditForm(const QString &title,
        const sokketter::socket_configuration &configuration, QWidget *parent = nullptr);
    ~SocketEditForm();

    auto configuration() const -> sokketter::socket_configuration;

private:
    Ui::SocketEditForm *m_ui;
};

#endif // SOCKETEDITFORM_H
