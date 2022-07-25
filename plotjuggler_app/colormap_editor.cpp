/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "colormap_editor.h"
#include "ui_colormap_editor.h"
#include "PlotJuggler/svg_util.h"
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include "stylesheet.h"
#include "color_map.h"

ColorMapEditor::ColorMapEditor(QWidget* parent)
  : QDialog(parent), ui(new Ui::colormap_editor)
{
  ui->setupUi(this);

  QSettings settings;

  ui->functionText->setHighlighter(new QLuaHighlighter);
  ui->functionText->setCompleter(new QLuaCompleter);

  auto theme = settings.value("StyleSheet::theme", "light").toString();
  on_stylesheetChanged(theme);

  for (const auto& it : ColorMapLibrary())
  {
    ui->listWidget->addItem(it.first);
  }
}

ColorMapEditor::~ColorMapEditor()
{
  SaveColorMapToSettings();
  delete ui;
}

void ColorMapEditor::on_stylesheetChanged(QString theme)
{
  ui->buttonDelete->setIcon(LoadSvg(":/resources/svg/trash.svg", theme));
  ui->functionText->setSyntaxStyle(GetLuaSyntaxStyle(theme));
}

void ColorMapEditor::on_buttonSave_clicked()
{
  bool ok;

  auto colormap = std::make_shared<ColorMap>();

  auto res = colormap->setScrip(ui->functionText->toPlainText());
  if (!res.valid())
  {
    QMessageBox::warning(this, "Error in the Lua Script", colormap->getError(res),
                         QMessageBox::Cancel);
    return;
  }

  QString default_name;
  auto selected = ui->listWidget->selectedItems();
  if (selected.size() == 1)
  {
    default_name = selected.front()->text();
  }

  QString name =
      QInputDialog::getText(this, tr("ColorMap Name"), tr("Name of the function:"),
                            QLineEdit::Normal, default_name, &ok);
  if (!ok || name.isEmpty())
  {
    return;
  }
  bool overwrite = false;

  if (!ui->listWidget->findItems(name, Qt::MatchExactly).empty())
  {
    auto reply = QMessageBox::question(this, "Confirm overwrite",
                                       "A ColorMap with the same name exist already. "
                                       "Do you want to overwrite it?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No)
    {
      return;
    }
    overwrite = true;
  }

  ColorMapLibrary()[name] = colormap;

  if (!overwrite)
  {
    ui->listWidget->addItem(name);
    selectRow(ui->listWidget->count() - 1);
  }
}

void ColorMapEditor::on_buttonDelete_clicked()
{
  for (auto item : ui->listWidget->selectedItems())
  {
    auto it = ColorMapLibrary().find(item->text());
    if (it != ColorMapLibrary().end())
    {
      if (!it->second->script().isEmpty())
      {
        auto reply = QMessageBox::question(this, "Delete ColorMap", "Are you sure?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
        {
          return;
        }
      }
      ColorMapLibrary().erase(it);
    }
    delete ui->listWidget->takeItem(ui->listWidget->row(item));
  }
}

void ColorMapEditor::selectRow(int row)
{
  ui->listWidget->clearSelection();
  ui->listWidget->selectionModel()->setCurrentIndex(
      ui->listWidget->model()->index(row, 0), QItemSelectionModel::SelectionFlag::Select);
}

void ColorMapEditor::on_listWidget_itemDoubleClicked(QListWidgetItem* item)
{
  auto it = ColorMapLibrary().find(item->text());
  if (it != ColorMapLibrary().end())
  {
    auto colormap = it->second;
    ui->functionText->setText(colormap->script());
  }
}
