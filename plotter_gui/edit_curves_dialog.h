#ifndef EDIT_CURVES_DIALOG_H
#define EDIT_CURVES_DIALOG_H

#include "series_data.h"

#include <QDialog>

namespace Ui {
class EditCurvesDialog;
}

class PlotWidget;

class EditCurvesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCurvesDialog(const PlotDataMapRef& plot_map_data,
                              bool isXYCurve,
                              PlotWidget *parent = nullptr);
    ~EditCurvesDialog();

    void addCurveName(const QString &name, const QColor &color);

    QString sourceX();
    QString sourceY();
    QString curveName();

private slots:
    void on_pushButtonClose_pressed();
    void on_pushButtonApply_pressed();
    void on_pushButtonAddNew_pressed();
    void on_pushButtonRemoveSelected_pressed();
    void on_pushButtonToggleCurveType_pressed();
    void on_listCurveWidget_itemSelectionChanged();

private:
    Ui::EditCurvesDialog *ui;
    const PlotDataMapRef& _plot_map_data;
    bool _isXYCurve;
    PlotWidget *_parent;
};

#endif // EDIT_CURVES_DIALOG_H
