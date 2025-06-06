#ifndef LICENSE_DIALOG_H
#define LICENSE_DIALOG_H

#include <QDialog>

namespace Ui {
    class license_dialog;
}

class license_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit license_dialog(QWidget *parent = nullptr);
    ~license_dialog();

private:
    Ui::license_dialog *m_ui;

    auto event(QEvent *event) -> bool override;
    auto setThemeAccordingToMode() -> void;
};

#endif // LICENSE_DIALOG_H
