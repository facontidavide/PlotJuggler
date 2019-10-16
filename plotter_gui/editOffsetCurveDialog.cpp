#include "editOffsetCurveDialog.h"
#include "ui_editOffset.h"
#include "plotwidget.h"
#include <QDebug>
#include "plotwidget.h"

EditOffsetCurveDialog::EditOffsetCurveDialog(PlotWidget *parent) :
 QDialog(parent),
 ui(new Ui::EditOffsetCurveDialog),
 _parent(parent)
{
    ui->setupUi(this);
}

EditOffsetCurveDialog::~EditOffsetCurveDialog()
{
    delete ui;
}

void EditOffsetCurveDialog::addCurveName(const QString &name, const QColor &color )
{
    QListWidgetItem* item = new QListWidgetItem( name );
    item->setForeground(color);
    ui->listCurveWidget->addItem(item);
}

void EditOffsetCurveDialog::on_pushButtonLoadEdit_pressed()
{
    auto selected_items = ui->listCurveWidget->selectedItems();

    for(const auto item: selected_items)
    {
        if( item->isHidden() == false)
        {
            // _parent->removeCurve( item->text().toStdString() );
            // item->setHidden( true );
            ui->lineEditOffsetX->setText( QString::number(_parent->GetOffsetXFromCurve( item->text().toStdString()),'g',10 ) ) ;
            ui->lineEditOffsetY->setText( QString::number(_parent->GetOffsetYFromCurve( item->text().toStdString()),'g',10 ) );
        }
    }

    if( ui->listCurveWidget->count() > 0)
    {
        _parent->replot();
    }
    closeIfEmpty();
}

void EditOffsetCurveDialog::on_pushButtonSaveEdit_pressed()
{
    auto selected_items = ui->listCurveWidget->selectedItems();

    for(const auto item: selected_items)
    {
        if( item->isHidden() == false)
        {
            // _parent->removeCurve( item->text().toStdString() );
            // item->setHidden( true );
            _parent->setOffsetFromCurve(item->text().toStdString(), ui->lineEditOffsetX->text().toDouble(), ui->lineEditOffsetY->text().toDouble());
        }
    }

    if( ui->listCurveWidget->count() > 0)
    {
        _parent->replot();
    }
    closeIfEmpty();
}

void EditOffsetCurveDialog::closeIfEmpty()
{
    bool isEmpty = true;
    for(int index = 0; index <ui->listCurveWidget->count(); ++index)
    {
        QListWidgetItem* item = ui->listCurveWidget->item( index );
        if( item->isHidden() == false)
        {
            isEmpty = false;
            break;
        }
    }
    if( isEmpty ) this->accept();
}
