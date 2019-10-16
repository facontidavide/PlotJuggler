#ifndef EditOffsetCurveDialog_H
#define EditOffsetCurveDialog_H

#include <QDialog>
#include <QListWidgetItem>
#include "qwt_plot_curve.h"

namespace Ui {
class EditOffsetCurveDialog;
}

class PlotWidget;

class EditOffsetCurveDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditOffsetCurveDialog(PlotWidget *parent);
    ~EditOffsetCurveDialog();

    void addCurveName(const QString& name, const QColor &color);

private slots:

    void on_pushButtonLoadEdit_pressed();
    void on_pushButtonSaveEdit_pressed();

private:
    Ui::EditOffsetCurveDialog *ui;

    void closeIfEmpty();

    PlotWidget *_parent;
};

#endif // EditOffsetCurveDialog_H
