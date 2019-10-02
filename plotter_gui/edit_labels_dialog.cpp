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

QString EditLabelsDialog::labelX()
{
    return _x_label;
}

QString EditLabelsDialog::labelY()
{
    return _y_label;
}

QString EditLabelsDialog::plotLabel()
{
    return _plot_label;
}

void EditLabelsDialog::setLabelX(const QString &label)
{
    _x_label = label;
    ui->editLabelX->setText(label);
}

void EditLabelsDialog::setLabelY(const QString &label)
{
    _y_label = label;
    ui->editLabelY->setText(label);
}

void EditLabelsDialog::setPlotLabel(const QString &label)
{
    _plot_label = label;
    ui->editPlotLabel->setText(label);
}
