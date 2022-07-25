/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef COLOR_MAP_EDITOR_H
#define COLOR_MAP_EDITOR_H

#include <QDialog>
#include <QListWidgetItem>

#include "QLuaCompleter"
#include "QLuaHighlighter"
#include "QSyntaxStyle"

namespace Ui
{
class colormap_editor;
}

class ColorMapEditor : public QDialog
{
  Q_OBJECT

public:
  explicit ColorMapEditor(QWidget* parent = nullptr);

  ~ColorMapEditor();

public slots:
  void on_stylesheetChanged(QString theme);

private slots:

  void on_buttonSave_clicked();

  void on_buttonDelete_clicked();

  void on_listWidget_itemDoubleClicked(QListWidgetItem* item);

private:
  Ui::colormap_editor* ui;

  void selectRow(int row);
};

#endif  // COLOR_MAP_EDITOR_H
