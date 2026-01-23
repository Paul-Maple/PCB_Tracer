#include "helpdialog.h"
#include "ui_helpdialog.h"

HelpDialog::HelpDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
    setWindowTitle("Справка");

    // Устанавливаем фиксированный размер окна
    setFixedSize(600, 500);

    // Запрещаем изменение размера окна
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
