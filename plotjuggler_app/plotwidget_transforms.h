#ifndef PLOTWIDGET_TRANSFORMS_H
#define PLOTWIDGET_TRANSFORMS_H

#include <QDialog>
#include "plotwidget.h"

namespace Ui {
class plotwidget_transforms;
}

class DialogTransformEditor : public QDialog
{
  Q_OBJECT

public:
  explicit DialogTransformEditor(PlotWidget* plotwidget);
  ~DialogTransformEditor();

private slots:

  void on_listCurves_itemSelectionChanged();

  void on_listTransforms_itemSelectionChanged();

  void on_pushButtonCancel_clicked();

  void on_pushButtonSave_clicked();

  void on_lineEditAlias_editingFinished();

private:
  Ui::plotwidget_transforms *ui;

  PlotWidget* _plotwidget;
  PlotWidget* _plotwidget_origin;

  std::set<QWidget*> _connected_transform_widgets;

  void setupTable();

  class RowWidget: public QWidget
  {
  public:
    RowWidget(QString text, QColor color);

    QString text() const;
    QColor color() const;

  private:
    QLabel* _text;
    QColor _color;
  };

};

#endif // PLOTWIDGET_TRANSFORMS_H
