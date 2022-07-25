/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef STATISTICS_DIALOG_H
#define STATISTICS_DIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include "PlotJuggler/plotdata.h"
#include "plotwidget.h"

namespace Ui
{
class statistics_dialog;
}

struct Statistics
{
  size_t count = 0;
  double min = 0;
  double max = 0;
  double mean_tot = 0;
};

class StatisticsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit StatisticsDialog(PlotWidget* parent = nullptr);
  ~StatisticsDialog();

  void update(Range range);

  void closeEvent(QCloseEvent* event);

  void setTitle(QString title);

  bool calcVisibleRange();

private:
  Ui::statistics_dialog* ui;

  PlotWidget* _parent;
};

#endif  // STATISTICS_DIALOG_H
