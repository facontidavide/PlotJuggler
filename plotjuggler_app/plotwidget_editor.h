/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PLOTWIDGET_EDITOR_H
#define PLOTWIDGET_EDITOR_H

#include <QDialog>
#include <QKeyEvent>
#include "plotwidget.h"
#include "color_wheel.hpp"
#include "color_preview.hpp"
#include "PlotJuggler/transform_function.h"

namespace Ui
{
class PlotWidgetEditor;
}

class EditorRowWidget : public QWidget
{
  Q_OBJECT

public:
  EditorRowWidget(QString text, QColor color);

  void enterEvent(QEvent* ev) override;
  void leaveEvent(QEvent* ev) override;

  QString text() const;

  void setColor(QColor color);
  QColor color() const;

signals:

  void deleteRow(QWidget* _this);

private:
  QLabel* _text;
  QColor _color;
  QPushButton* _delete_button;
  QWidget* _empty_spacer;
};

class PlotwidgetEditor : public QDialog
{
  Q_OBJECT

public:
  explicit PlotwidgetEditor(PlotWidget* plotwidget, QWidget* parent = nullptr);
  ~PlotwidgetEditor();

public slots:
  void onColorChanged(QColor c);

private slots:

  void on_editColotText_textChanged(const QString& arg1);

  void on_radioLines_toggled(bool checked);

  void on_radioPoints_toggled(bool checked);

  void on_radioBoth_toggled(bool checked);

  void on_checkBoxMax_toggled(bool checked);

  void on_checkBoxMin_toggled(bool checked);

  void on_pushButtonReset_clicked();

  void on_pushButtonCancel_pressed();

  void on_pushButtonSave_pressed();

  void on_listWidget_itemSelectionChanged();

  void on_lineLimitMin_textChanged(const QString& text);

  void on_lineLimitMax_textChanged(const QString& text);

  void on_radioSticks_toggled(bool checked);

  void on_radioSteps_toggled(bool checked);

  void on_radioStepsInv_toggled(bool checked);

private:
  Ui::PlotWidgetEditor* ui;

  color_widgets::ColorWheel* _color_wheel;
  color_widgets::ColorPreview* _color_preview;
  PlotWidget* _plotwidget;
  PlotWidget* _plotwidget_origin;
  QRectF _bounding_rect_original;

  std::set<QWidget*> _connected_transform_widgets;

  void setupColorWidget();
  void setupTable();
  void updateLimits();
  void onDeleteRow(QWidget* w);
  void disableWidgets();

  std::unordered_map<std::string, std::shared_ptr<TransformFunction>> _transforms;
};

#endif  // PLOTWIDGET_EDITOR_H
