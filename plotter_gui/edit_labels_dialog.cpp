#include "edit_labels_dialog.h"
#include "ui_edit_labels_dialog.h"

#include <QLineEdit>

EditLabelsDialog::EditLabelsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditLabelsDialog)
{
    ui->setupUi(this);
}

EditLabelsDialog::~EditLabelsDialog()
{
    delete ui;
}


void EditLabelsDialog::on_pushButtonDone_pressed()
{
    _plot_label = ui->editPlotLabel->text();
    _x_label = ui->editLabelX->text();
    _y_label = ui->editLabelY->text();
    this->accept();
}

void EditLabelsDialog::on_pushButtonCancel_pressed()
{
    this->reject();
}

QString EditLabelsDialog::plotLabel()
{
    return _plot_label;
}

QString EditLabelsDialog::labelX()
{
    return _x_label;
}

QString EditLabelsDialog::labelY()
{
    return _y_label;
}
