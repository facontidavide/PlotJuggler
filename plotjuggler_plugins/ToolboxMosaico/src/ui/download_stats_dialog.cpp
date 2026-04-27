/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "download_stats_dialog.h"

#include "format_utils.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

DownloadStatsDialog::DownloadStatsDialog(QWidget* parent) : QDialog(parent)
{
  setWindowTitle(QStringLiteral("Download Statistics"));
  setWindowModality(Qt::WindowModal);
  resize(720, 360);

  auto* root = new QVBoxLayout(this);
  summary_label_ = new QLabel(this);
  summary_label_->setWordWrap(true);
  root->addWidget(summary_label_);

  grid_ = new QGridLayout();
  grid_->setColumnStretch(0, 2);
  grid_->setColumnStretch(1, 3);
  grid_->setColumnStretch(2, 1);
  grid_->setColumnStretch(3, 1);
  grid_->setColumnStretch(4, 1);
  grid_->addWidget(new QLabel(QStringLiteral("Topic"), this), 0, 0);
  grid_->addWidget(new QLabel(QStringLiteral("Progress"), this), 0, 1);
  grid_->addWidget(new QLabel(QStringLiteral("Bytes"), this), 0, 2);
  grid_->addWidget(new QLabel(QStringLiteral("Speed"), this), 0, 3);
  grid_->addWidget(new QLabel(QStringLiteral("Status"), this), 0, 4);
  root->addLayout(grid_);

  auto* buttons = new QDialogButtonBox(this);
  cancel_button_ = buttons->addButton(QStringLiteral("Cancel"), QDialogButtonBox::RejectRole);
  root->addWidget(buttons);
  connect(cancel_button_, &QPushButton::clicked, this, &DownloadStatsDialog::reject);

  timer_ = new QTimer(this);
  timer_->setInterval(1000);
  connect(timer_, &QTimer::timeout, this, &DownloadStatsDialog::refreshStats);
}

void DownloadStatsDialog::start(const QStringList& topics)
{
  clearRows();
  active_ = true;
  cancelling_ = false;
  previous_total_bytes_ = 0;
  elapsed_.restart();
  cancel_button_->setText(QStringLiteral("Cancel"));
  cancel_button_->setEnabled(true);

  int row = 1;
  for (const auto& topic : topics)
  {
    Row item;
    item.topic = new QLabel(topic, this);
    item.topic->setTextInteractionFlags(Qt::TextSelectableByMouse);
    item.progress = new QProgressBar(this);
    item.progress->setRange(0, 0);
    item.bytes = new QLabel(QStringLiteral("0 B"), this);
    item.speed = new QLabel(QStringLiteral("0 B/s"), this);
    item.status = new QLabel(QStringLiteral("Waiting"), this);

    grid_->addWidget(item.topic, row, 0);
    grid_->addWidget(item.progress, row, 1);
    grid_->addWidget(item.bytes, row, 2);
    grid_->addWidget(item.speed, row, 3);
    grid_->addWidget(item.status, row, 4);
    rows_.insert(topic, item);
    ++row;
  }

  summary_label_->setText(QStringLiteral(
      "Network bytes and speed are decoded Arrow Flight payload bytes. The current Arrow Flight "
      "reader does not expose exact wire traffic."));
  timer_->start();
  refreshStats();
}

void DownloadStatsDialog::updateProgress(const QString& topic, qint64 bytes, qint64 total_bytes,
                                         bool from_network)
{
  Row* row = rowForTopic(topic);
  if (!row)
  {
    return;
  }

  if (from_network)
  {
    row->network_bytes = bytes;
  }
  else
  {
    row->cache_bytes = bytes;
  }
  row->total_bytes = total_bytes;
  if (from_network && total_bytes > 0)
  {
    row->bytes->setText(QStringLiteral("%1 / %2").arg(formatBytes(bytes),
                                                      formatBytes(total_bytes)));
    row->progress->setRange(0, 1000);
    row->progress->setValue(static_cast<int>((1000 * bytes) / total_bytes));
  }
  else
  {
    row->bytes->setText(from_network ? formatBytes(bytes) :
                                       QStringLiteral("cache %1").arg(formatBytes(bytes)));
  }
  if (!row->finished)
  {
    row->status->setText(from_network ? QStringLiteral("Downloading") :
                                        QStringLiteral("Cache hit"));
  }
}

void DownloadStatsDialog::markFinished(const QString& topic, const QString& status)
{
  Row* row = rowForTopic(topic);
  if (!row)
  {
    return;
  }
  const bool cache_only = row->cache_bytes > 0 && row->network_bytes == 0;
  const QString display_status =
      status == QStringLiteral("Done") && cache_only ? QStringLiteral("Cached") : status;
  row->finished = true;
  row->status->setText(display_status);
  row->progress->setRange(0, 1000);
  row->progress->setValue((display_status == QStringLiteral("Done") ||
                           display_status == QStringLiteral("Cached")) ?
                              1000 :
                              0);
  refreshStats();
}

void DownloadStatsDialog::markCancelling()
{
  cancelling_ = true;
  cancel_button_->setEnabled(false);
  cancel_button_->setText(QStringLiteral("Cancelling..."));
  for (auto it = rows_.begin(); it != rows_.end(); ++it)
  {
    if (!it->finished)
    {
      it->status->setText(QStringLiteral("Cancelling"));
    }
  }
}

void DownloadStatsDialog::markComplete()
{
  active_ = false;
  cancelling_ = false;
  timer_->stop();
  cancel_button_->setEnabled(true);
  cancel_button_->setText(QStringLiteral("Close"));
  refreshStats();
}

void DownloadStatsDialog::reject()
{
  if (active_)
  {
    markCancelling();
    emit cancelRequested();
    return;
  }
  QDialog::reject();
}

void DownloadStatsDialog::refreshStats()
{
  qint64 network_total = 0;
  for (auto it = rows_.begin(); it != rows_.end(); ++it)
  {
    network_total += it->network_bytes;
    const qint64 delta = it->network_bytes - it->previous_network_bytes;
    it->speed->setText(formatBytes(std::max<qint64>(0, delta)) + QStringLiteral("/s"));
    it->previous_network_bytes = it->network_bytes;
  }

  const qint64 total_delta = network_total - previous_total_bytes_;
  previous_total_bytes_ = network_total;
  const QString state = cancelling_ ? QStringLiteral("Cancelling") :
                        active_     ? QStringLiteral("Downloading") :
                                      QStringLiteral("Complete");
  summary_label_->setText(QStringLiteral("%1 - network payload %2, current network speed %3/s. "
                                         "Exact wire traffic is not exposed by the current Arrow "
                                         "Flight reader.")
                              .arg(state, formatBytes(network_total),
                                   formatBytes(std::max<qint64>(0, total_delta))));
}

DownloadStatsDialog::Row* DownloadStatsDialog::rowForTopic(const QString& topic)
{
  auto it = rows_.find(topic);
  if (it == rows_.end())
  {
    return nullptr;
  }
  return &(*it);
}

void DownloadStatsDialog::clearRows()
{
  for (auto it = rows_.begin(); it != rows_.end(); ++it)
  {
    delete it->topic;
    delete it->progress;
    delete it->bytes;
    delete it->speed;
    delete it->status;
  }
  rows_.clear();
}
