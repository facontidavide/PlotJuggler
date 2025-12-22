/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "plotwidget_transforms.h"
#include "ui_plotwidget_transforms.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QListWidgetItem>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDebug>
#include <qwt_text.h>

DialogTransformEditor::DialogTransformEditor(PlotWidget* plotwidget)
  : QDialog(plotwidget), ui(new Ui::plotwidget_transforms), _plotwidget_origin(plotwidget)
{
  ui->setupUi(this);

  QDomDocument doc;
  auto saved_state = plotwidget->xmlSaveState(doc);
  _plotwidget = new PlotWidget(plotwidget->datamap(), this);
  _plotwidget->on_changeTimeOffset(plotwidget->timeOffset());
  _plotwidget->xmlLoadState(saved_state);

  auto layout = new QVBoxLayout();
  ui->framePlotPreview->setLayout(layout);
  layout->addWidget(_plotwidget);
  layout->setMargin(6);

  _plotwidget->zoomOut(false);
  _plotwidget->setContextMenuEnabled(false);

  setupTable();

  QSettings settings;
  restoreGeometry(settings.value("DialogTransformEditor.geometry").toByteArray());

  ui->listCurves->setStyleSheet("QListView::item:selected { background: #ddeeff; }");

  auto names = TransformFactory::registeredTransforms();

  for (const auto& name : names)
  {
    ui->listTransforms->addItem(QString::fromStdString(name));
  }

  if (ui->listCurves->count() != 0)
  {
    ui->listCurves->item(0)->setSelected(true);
  }
}

void DialogTransformEditor::setupTable()
{
  std::map<QString, QColor> colors = _plotwidget->getCurveColors();

  int row = 0;
  for (auto& [curve_name, color] : colors)
  {
    auto curve_it = _plotwidget->curveFromTitle(curve_name);
    auto name = QString::fromStdString(curve_it->src_name);
    auto item = new QListWidgetItem();
    //  item->setForeground(color);
    ui->listCurves->addItem(item);
    auto plot_row = new RowWidget(name, color);
    item->setSizeHint(plot_row->sizeHint());
    ui->listCurves->setItemWidget(item, plot_row);
    row++;
  }
}

DialogTransformEditor::~DialogTransformEditor()
{
  QSettings settings;
  settings.setValue("DialogTransformEditor.geometry", saveGeometry());

  delete ui;
}

DialogTransformEditor::RowWidget::RowWidget(QString text, QColor color)
{
  auto layout = new QHBoxLayout();
  setLayout(layout);
  _text = new QLabel(text, this);

  setStyleSheet(QString("color: %1;").arg(color.name()));
  _color = color;

  layout->addWidget(_text);
}

QString DialogTransformEditor::RowWidget::text() const
{
  return _text->text();
}

QColor DialogTransformEditor::RowWidget::color() const
{
  return _color;
}

void DialogTransformEditor::on_listCurves_itemSelectionChanged()
{
  auto selected_curves = ui->listCurves->selectedItems();
  if (selected_curves.size() < 1)
  {
    return;
  }
  if (selected_curves.size() > 1)
  {
    // multi-selected curves may have different transforms
    ui->listTransforms->clearSelection();
    return;
  }

  auto row_widget = dynamic_cast<RowWidget*>(ui->listCurves->itemWidget(selected_curves.front()));
  auto curve_name = row_widget->text();
  auto curve_info = _plotwidget->curveFromTitle(curve_name);

  int transform_row = 0;
  if (auto ts = dynamic_cast<TransformedTimeseries*>(curve_info->curve->data()))
  {
    if (ts->transform())
    {
      for (int row = 1; row < ui->listTransforms->count(); row++)
      {
        if (ui->listTransforms->item(row)->text() == ts->transformName())
        {
          transform_row = row;
          break;
        }
      }
    }
  }

  int selected_row = -1;
  auto selected_transforms = ui->listTransforms->selectedItems();
  if (selected_transforms.size() == 1)
  {
    selected_row = ui->listTransforms->row(selected_transforms.front());
  }

  if (selected_row == transform_row)
  {
    // force callback
    on_listTransforms_itemSelectionChanged();
  }
  else
  {
    ui->listTransforms->item(transform_row)->setSelected(true);
  }
}

void DialogTransformEditor::on_listTransforms_itemSelectionChanged()
{
  auto selected_curves = ui->listCurves->selectedItems();
  if (selected_curves.size() < 1)
  {
    return;
  }

  auto selected_transforms = ui->listTransforms->selectedItems();
  if (selected_transforms.size() != 1)
  {
    ui->stackedWidgetArguments->setCurrentIndex(0);
    return;
  }

  QSignalBlocker block(ui->lineEditAlias);

  QString transform_ID = selected_transforms.front()->text();
  if (transform_ID == ui->listTransforms->item(0)->text())
    transform_ID.clear();

  if (transform_ID.isEmpty())
  {
    ui->stackedWidgetArguments->setCurrentIndex(0);
  }

  if (transform_ID.isEmpty() || selected_curves.size() > 1)
  {
    ui->lineEditAlias->setText("");
    ui->lineEditAlias->setEnabled(false);
  }

  TransformedTimeseries* ts = nullptr;

  for (auto item : selected_curves)
  {
    auto row_widget = dynamic_cast<RowWidget*>(ui->listCurves->itemWidget(item));
    QString curve_name = row_widget->text();
    auto curve_info = _plotwidget->curveFromTitle(curve_name);
    auto qwt_curve = curve_info->curve;
    ts = dynamic_cast<TransformedTimeseries*>(curve_info->curve->data());

    auto src_name = QString::fromStdString(curve_info->src_name);
    bool has_default_title =
        ts->alias().isEmpty() ||
        ts->alias().compare(QString("%1[%2]").arg(src_name).arg(ts->transformName())) == 0;

    if (transform_ID.isEmpty())
    {
      ts->setTransform({});
      ts->updateCache(true);

      if (has_default_title)
      {
        ts->setAlias(QString());
      }

      qwt_curve->setTitle(curve_name);
    }
    else
    {
      ts->setTransform(transform_ID);
      ts->updateCache(true);

      if (has_default_title)
      {
        auto new_default_title = QString("%1[%2]").arg(src_name).arg(transform_ID);
        ts->setAlias(new_default_title);
      }

      qwt_curve->setTitle(ts->alias());

      if (selected_curves.size() == 1)
      {
        ui->lineEditAlias->setText(ts->alias());
        ui->lineEditAlias->setEnabled(true);
      }
    }
  }

  // use the last selected curve, as the transform widget presenter
  if (ts && ts->transform())
  {
    QWidget* widget = ts->transform()->optionsWidget();

    if (widget)
    {
      int index = ui->stackedWidgetArguments->indexOf(widget);
      if (index == -1 && widget)
      {
        index = ui->stackedWidgetArguments->addWidget(widget);
      }

      ui->stackedWidgetArguments->setCurrentIndex(index);

      if (_connected_transform_widgets.count(widget) == 0)
      {
        connect(ts->transform().get(), &TransformFunction::parametersChanged, this, [this, ts]() {
          // update this transform
          ts->updateCache(true);

          // update the others if necessary
          if (ui->listCurves->selectedItems().size() > 1)
          {
            // Copy state from visible widget and apply to all selected curves.
            QDomDocument doc;
            QDomElement transform_state = doc.createElement("transform");
            ts->transform()->xmlSaveState(doc, transform_state);

            for (auto item : ui->listCurves->selectedItems())
            {
              auto row_widget = dynamic_cast<RowWidget*>(ui->listCurves->itemWidget(item));
              QString curve_name = row_widget->text();
              auto curve_info = _plotwidget->curveFromTitle(curve_name);
              auto* item_ts = dynamic_cast<TransformedTimeseries*>(curve_info->curve->data());

              if (item_ts != ts)
              {
                QSignalBlocker block_transform(item_ts->transform().get());
                item_ts->transform()->xmlLoadState(transform_state);
                item_ts->updateCache(true);
              }
            }
          }
          // replot
          if (ui->checkBoxAutoZoom->isChecked())
          {
            _plotwidget->zoomOut(false);
          }
          else
          {
            _plotwidget->replot();
          }
        });
        _connected_transform_widgets.insert(widget);
      }
    }
  }

  _plotwidget->zoomOut(false);
}

void DialogTransformEditor::on_pushButtonCancel_clicked()
{
  this->reject();
}

void DialogTransformEditor::on_pushButtonSave_clicked()
{
  on_lineEditAlias_editingFinished();

  QSettings settings;
  bool autozoom_filter_applied =
      settings.value("Preferences::autozoom_filter_applied", true).toBool();
  QDomDocument doc;
  auto elem = _plotwidget->xmlSaveState(doc);
  _plotwidget_origin->xmlLoadState(elem, autozoom_filter_applied);

  if (autozoom_filter_applied)
  {
    _plotwidget_origin->zoomOut(false);
  }

  this->accept();
}

void DialogTransformEditor::on_lineEditAlias_editingFinished()
{
  auto selected_curves = ui->listCurves->selectedItems();
  if (selected_curves.size() != 1)
  {
    return;
  }
  auto row_widget = dynamic_cast<RowWidget*>(ui->listCurves->itemWidget(selected_curves.front()));

  QString curve_name = row_widget->text();

  auto curve_it = _plotwidget->curveFromTitle(curve_name);
  auto ts = dynamic_cast<TransformedTimeseries*>(curve_it->curve->data());

  curve_it->curve->setTitle(ui->lineEditAlias->text());

  if (ts && ts->transform())
  {
    ts->setAlias(ui->lineEditAlias->text());
  }

  _plotwidget->replot();
}
