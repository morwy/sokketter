#ifndef DEVICE_EDIT_FORM_H
#define DEVICE_EDIT_FORM_H

#pragma once

#include <libsokketter.h>

#include <QWidget>

namespace Ui {
    class DeviceEditForm;
}

class DeviceEditForm : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceEditForm(
        const sokketter::power_strip_configuration &configuration, QWidget *parent = nullptr);
    ~DeviceEditForm();

    auto configuration() const -> sokketter::power_strip_configuration;

private:
    Ui::DeviceEditForm *m_ui;
};

#endif // DEVICE_EDIT_FORM_H
