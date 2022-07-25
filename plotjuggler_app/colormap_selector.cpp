/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "colormap_selector.h"
#include "ui_colormap_selector.h"
#include "color_map.h"
#include "colormap_editor.h"
#include <QSettings>

ColormapSelectorDialog::ColormapSelectorDialog(QString series, QString default_colormap,
                                               QWidget* parent)
  : QDialog(parent), ui(new Ui::colormap_selector)
{
  ui->setupUi(this);
  ui->lineSeries->setText(series);

  if (default_colormap.isEmpty())
  {
    QSettings settings;
    default_colormap =
        settings.value("ColormapSelectorDialog::prevColorMap", "").toString();
  }

  for (auto it : ColorMapLibrary())
  {
    ui->comboBox->addItem(it.first);
  }
  if (ColorMapLibrary().count(default_colormap))
  {
    ui->comboBox->setCurrentText(default_colormap);
  }

  auto reset_button = ui->buttonBox->button(QDialogButtonBox::Reset);
  connect(reset_button, &QPushButton::clicked, this, [this]() {
    _selected.clear();
    this->accept();
  });
}

ColormapSelectorDialog::~ColormapSelectorDialog()
{
  QSettings settings;
  settings.setValue("ColormapSelectorDialog::prevColorMap", ui->comboBox->currentText());

  delete ui;
}

QString ColormapSelectorDialog::selectedColorMap() const
{
  return _selected;
}

void ColormapSelectorDialog::on_buttonEditor_clicked()
{
  ColorMapEditor dialog;
  dialog.exec();

  QString default_colormap = ui->comboBox->currentText();

  ui->comboBox->clear();
  for (auto it : ColorMapLibrary())
  {
    ui->comboBox->addItem(it.first);
  }
  if (ColorMapLibrary().count(default_colormap))
  {
    ui->comboBox->setCurrentText(default_colormap);
  }
}

void ColormapSelectorDialog::on_comboBox_currentTextChanged(const QString& name)
{
  _selected = name;
}
