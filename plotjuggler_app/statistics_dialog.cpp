/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "statistics_dialog.h"
#include "ui_statistics_dialog.h"
#include <QTableWidgetItem>
#include "qwt_text.h"

StatisticsDialog::StatisticsDialog(PlotWidget* parent)
  : QDialog(parent), ui(new Ui::statistics_dialog), _parent(parent)
{
  ui->setupUi(this);

  setWindowTitle(QString("Statistics | %1").arg(_parent->windowTitle()));
  setWindowFlag(Qt::Tool);

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

  connect(ui->rangeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
          [this]() {
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
        stat.min = p.y();
        stat.max = p.y();
        first = false;
      }
      else
      {
        stat.min = std::min(stat.min, p.y());
        stat.max = std::max(stat.max, p.y());
      }
      stat.mean_tot += p.y();
    }
    statistics[info.curve->title().text()] = stat;
  }

  ui->tableWidget->setRowCount(statistics.size());
  int row = 0;
  for (const auto& it : statistics)
  {
    const auto& stat = it.second;

    std::array<QString, 5> row_values;
    row_values[0] = it.first;
    row_values[1] = QString::number(stat.count);
    row_values[2] = QString::number(stat.min, 'f');
    row_values[3] = QString::number(stat.max, 'f');
    double mean = stat.mean_tot / double(stat.count);
    row_values[4] = QString::number(mean, 'f');

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
