#include "edit_curves_dialog.h"
#include "ui_edit_curves_dialog.h"
#include "plotwidget.h"
#include "PlotJuggler/random_color.h"

#include <QLineEdit>
#include <QDebug>

EditCurvesDialog::EditCurvesDialog(const PlotDataMapRef& plot_map_data,
                                   bool isXYCurve,
                                   PlotWidget *parent) :
    QDialog(parent),
    _plot_map_data(plot_map_data),
    _isXYCurve(isXYCurve),
    _parent(parent),
    ui(new Ui::EditCurvesDialog)
{
    ui->setupUi(this);

    ui->pushButtonToggleCurveType->setText("Make Timeseries");
    if (!_isXYCurve)
    {
        ui->pushButtonToggleCurveType->setText("Make XY Plot");
        ui->comboButtonBrowseX->setEnabled(_isXYCurve);
    }

    QStringList numericPlotNames;
    for(const auto &p : _plot_map_data.numeric)
    {
        QString name = QString::fromStdString(p.first);
        numericPlotNames.push_back(name);
    }
    numericPlotNames.sort(Qt::CaseInsensitive);
    ui->comboButtonBrowseX->addItem("");
    ui->comboButtonBrowseY->addItem("");
    for(const QString& name : numericPlotNames)
    {
        ui->comboButtonBrowseX->addItem(name);
        ui->comboButtonBrowseY->addItem(name);
    }
}

EditCurvesDialog::~EditCurvesDialog()
{
    delete ui;
}

void EditCurvesDialog::addCurveName(const QString &curve_name, 
                                    const QColor &color )
{
    QListWidgetItem* item = new QListWidgetItem(curve_name);
    item->setForeground(color);
    ui->listCurveWidget->addItem(item);
}

void EditCurvesDialog::on_listCurveWidget_itemSelectionChanged()
{
    // Using currentItem() is not strictly correct. Should find the
    // right way of doing this.

    auto current_item = ui->listCurveWidget->currentItem();
    if (current_item == NULL)
    {
        return;
    }

    if (_isXYCurve)
    {
        std::string orig = current_item->text().toStdString();
        std::string str = orig;
        std::string delimiter = "[";
        std::string xtopic, ytopic;
        size_t pos = 0;
        while ((pos = str.find(delimiter)) != std::string::npos) 
        {
            xtopic = ytopic = str.substr(0, pos);
            str.erase(0, pos + delimiter.length());
        }
        delimiter = ";";
        while ((pos = str.find(delimiter)) != std::string::npos) 
        {
            xtopic += str.substr(0, pos);
            str.erase(0, pos + delimiter.length());
            ytopic += str.substr(0, str.length()-1);
        }
        ui->comboButtonBrowseX->setCurrentText(QString::fromStdString(xtopic));
        ui->comboButtonBrowseY->setCurrentText(QString::fromStdString(ytopic));
    }
    else
    {
        ui->comboButtonBrowseY->setCurrentText(current_item->text());
    }
}

void EditCurvesDialog::on_pushButtonClose_pressed()
{
    this->accept();
}

void EditCurvesDialog::on_pushButtonApply_pressed()
{
    if (_isXYCurve)
    {
        auto xtopic = ui->comboButtonBrowseX->currentText().toStdString();
        auto ytopic = ui->comboButtonBrowseY->currentText().toStdString();
        auto current_item = ui->listCurveWidget->currentItem();
        if (current_item == NULL)
        {
            return;
        }

        size_t pos = 0;
        while (xtopic[pos] == ytopic[pos])
        {
            pos++;
        }
        auto base = xtopic.substr(0, pos);
        auto x_name = xtopic;
        auto y_name = ytopic;
        x_name = x_name.erase(0, pos);
        y_name = y_name.erase(0, pos);
        auto new_name = QString::fromStdString(base + "[" + x_name + ";" + y_name + "]");

        if (!ui->listCurveWidget->findItems(new_name, 0).empty()
            || xtopic == ytopic
            || xtopic == ""
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        _parent->removeCurve(current_item->text().toStdString());
        _parent->addCurveXY(xtopic, ytopic, new_name, current_item->foreground().color());

        current_item->setText(new_name);
        _parent->replot();
    }
    else
    {
        auto ytopic = ui->comboButtonBrowseY->currentText();
        auto current_item = ui->listCurveWidget->currentItem();
        if (current_item == NULL)
        {
            return;
        }

        if (!ui->listCurveWidget->findItems(ytopic, 0).empty()
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        _parent->removeCurve(current_item->text().toStdString());
        _parent->addCurve(ytopic.toStdString(), current_item->foreground().color());

        current_item->setText(ytopic);
        _parent->replot();
    }
}

void EditCurvesDialog::on_pushButtonAddNew_pressed()
{
    if (_isXYCurve)
    {
        auto xtopic = ui->comboButtonBrowseX->currentText().toStdString();
        auto ytopic = ui->comboButtonBrowseY->currentText().toStdString();

        size_t pos = 0;
        while (xtopic[pos] == ytopic[pos])
        {
            pos++;
        }
        auto base = xtopic.substr(0, pos);
        auto x_name = xtopic;
        auto y_name = ytopic;
        x_name = x_name.erase(0, pos);
        y_name = y_name.erase(0, pos);
        auto new_name = QString::fromStdString(base + "[" + x_name + ";" + y_name + "]");

        if (!ui->listCurveWidget->findItems(new_name, 0).empty()
            || xtopic == ytopic
            || xtopic == ""
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        auto color = randomColorHint();
        _parent->addCurveXY(xtopic, ytopic, new_name, color);
        addCurveName(new_name, color);
        _parent->replot();
    }
    else
    {
        auto ytopic = ui->comboButtonBrowseY->currentText();

        if (!ui->listCurveWidget->findItems(ytopic, 0).empty()
            || ytopic == "")
        {
            QMessageBox::warning(this, "Invalid Curve",
                                 "The proposed curve is either invalid or already exists.",
                                 QMessageBox::Ok);
            return;
        }

        auto color = randomColorHint();
        _parent->addCurve(ytopic.toStdString(), color);
        addCurveName(ytopic, color);
        _parent->replot();
    }
}

void EditCurvesDialog::on_pushButtonRemoveSelected_pressed()
{
    if (ui->listCurveWidget->count() == 0)
    {
        return;
    }

    // Again, not strictly correct.
    auto current_item = ui->listCurveWidget->currentItem();
    if (current_item->isHidden() == false)
    {
        _parent->removeCurve(current_item->text().toStdString());
        current_item->setHidden( true );
    }
    _parent->replot();
}

void EditCurvesDialog::on_pushButtonToggleCurveType_pressed()
{
    int count = ui->listCurveWidget->count();
    if (count != 0)
    {
        auto ret = QMessageBox::warning(this, "Warning",
          "Toggling plot types will remove all existing curves. Proceed?",
          QMessageBox::Ok | QMessageBox::Cancel);
        if (ret != QMessageBox::Ok)
        {
            return;
        }
count;
        for (int i=0; i < count; i++)
        {
            auto item = ui->listCurveWidget->takeItem(0);
            _parent->removeCurve(item->text().toStdString());
            delete item;
        }
    }

    if (_isXYCurve)
    {
        ui->pushButtonToggleCurveType->setText("Make XY Plot");
        _parent->convertToTimeseries();
    }
    else
    {
        ui->pushButtonToggleCurveType->setText("Make Timeseries");
        _parent->convertToXY();
    }

    _isXYCurve = !_isXYCurve;
    ui->comboButtonBrowseX->setEnabled(_isXYCurve);
    ui->comboButtonBrowseX->setCurrentText("");
    ui->comboButtonBrowseY->setCurrentText("");

}
