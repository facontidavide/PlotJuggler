/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "statistics_dialog.h"
#include "ui_statistics_dialog.h"
#include <QTableWidgetItem>
#include <cmath>
#include "qwt_text.h"

StatisticsDialog::StatisticsDialog(PlotWidget* parent)
  : QDialog(parent), ui(new Ui::statistics_dialog), _parent(parent)
{
  ui->setupUi(this);

  setWindowTitle(QString("Statistics | %1").arg(_parent->windowTitle()));
  setWindowFlag(Qt::Tool);

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  connect(ui->rangeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
    auto rect = _parent->currentBoundingRect();
    update({ rect.left(), rect.right() });
  });
}

StatisticsDialog::~StatisticsDialog()
{
  delete ui;
}

bool StatisticsDialog::calcVisibleRange()
{
  return (ui->rangeComboBox->currentIndex() == 0);
}

void StatisticsDialog::update(PJ::Range range)
{
  std::map<QString, Statistics> statistics;

  for (const auto& info : _parent->curveList())
  {
    Statistics stat;
    double start_time;
    double end_time;
    const auto ts = info.curve->data();

    bool first = true;

    for (size_t i = 0; i < ts->size(); i++)
    {
      const auto p = ts->sample(i);
      if (calcVisibleRange())
      {
        if (p.x() < range.min)
        {
          continue;
        }
        if (p.x() > range.max)
        {
          break;
        }
      }
      stat.count++;
      if (first)
      {
        start_time = p.x();
        end_time = p.x();
        stat.min = p.y();
        stat.max = p.y();
        first = false;
      }
      else
      {
        start_time = std::min(start_time, p.x());
        end_time = std::max(end_time, p.x());
        stat.min = std::min(stat.min, p.y());
        stat.max = std::max(stat.max, p.y());
      }
      stat.mean_tot += p.y();
      stat.square_tot += p.y() * p.y();
      stat.abs_tot += std::fabs(p.y());
    }

    if (stat.count > 0)
    {
      stat.mean_interval = (end_time - start_time) / double(stat.count - 1);
    }

    statistics[info.curve->title().text()] = stat;
  }

  ui->tableWidget->setRowCount(statistics.size());
  int row = 0;
  for (const auto& it : statistics)
  {
    const auto& stat = it.second;

    std::array<QString, 10> row_values;
    row_values[0] = it.first;
    row_values[1] = QString::number(stat.count);
    row_values[2] = QString::number(stat.min, 'f');
    row_values[3] = QString::number(stat.max, 'f');
    const double peak_to_peak = (stat.count > 0) ? (stat.max - stat.min) : 0.0;
    double mean = 0;
    double rms = 0;
    double stddev = 0;
    double mean_abs_value = 0;
    if (stat.count > 0)
    {
      const double count = double(stat.count);
      mean = stat.mean_tot / count;
      rms = std::sqrt(stat.square_tot / count);
      mean_abs_value = stat.abs_tot / count;
      const double variance = std::max(0.0, (stat.square_tot / count) - (mean * mean));
      stddev = std::sqrt(variance);
    }

    row_values[4] = QString::number(peak_to_peak, 'f');
    row_values[5] = QString::number(mean, 'f');
    row_values[6] = QString::number(rms, 'f');
    row_values[7] = QString::number(stddev, 'f');
    row_values[8] = QString::number(mean_abs_value, 'f');
    row_values[9] = QString::number(stat.mean_interval, 'f');

    for (size_t col = 0; col < row_values.size(); col++)
    {
      if (auto item = ui->tableWidget->item(row, col))
      {
        item->setText(row_values[col]);
      }
      else
      {
        ui->tableWidget->setItem(row, col, new QTableWidgetItem(row_values[col]));
      }
    }
    row++;
  }
}

void StatisticsDialog::setTitle(QString title)
{
  if (title == "...")
  {
    title = "";
  }
  setWindowTitle(QString("Statistics | %1").arg(title));
}

void StatisticsDialog::closeEvent(QCloseEvent* event)
{
  QWidget::closeEvent(event);
  emit rejected();
}
