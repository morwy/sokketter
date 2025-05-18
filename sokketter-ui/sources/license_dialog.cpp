#include "license_dialog.h"
#include "ui_license_dialog.h"

license_dialog::license_dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::license_dialog)
{
    ui->setupUi(this);
}

license_dialog::~license_dialog()
{
    delete ui;
}
