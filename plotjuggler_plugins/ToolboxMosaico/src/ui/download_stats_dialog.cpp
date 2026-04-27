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
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
constexpr qint64 kSpeedWindowMs = 5000;

QString formatByteCount(qint64 bytes)
{
  return bytes > 0 ? formatBytes(bytes) : QStringLiteral("0 B");
}

QString formatSpeed(qint64 bytes_per_second)
{
  return QStringLiteral("%1/s").arg(formatByteCount(bytes_per_second));
}

QLabel* makeHeaderLabel(const QString& text, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  auto font = label->font();
  font.setBold(true);
  label->setFont(font);
  return label;
}
}  // namespace

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
  grid_->setColumnStretch(1, 1);
  grid_->setColumnStretch(2, 1);
  grid_->setColumnStretch(3, 1);
  grid_->addWidget(makeHeaderLabel(QStringLiteral("Topic"), this), 0, 0);
  grid_->addWidget(makeHeaderLabel(QStringLiteral("Bytes"), this), 0, 1);
  grid_->addWidget(makeHeaderLabel(QStringLiteral("Speed"), this), 0, 2);
  grid_->addWidget(makeHeaderLabel(QStringLiteral("Status"), this), 0, 3);
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
  total_speed_samples_.clear();
  elapsed_.restart();
  cancel_button_->setText(QStringLiteral("Cancel"));
  cancel_button_->setEnabled(true);

  int row = 1;
  for (const auto& topic : topics)
  {
    Row item;
    item.topic = new QLabel(topic, this);
    item.topic->setTextInteractionFlags(Qt::TextSelectableByMouse);
    item.bytes = new QLabel(QStringLiteral("0 B"), this);
    item.speed = new QLabel(QStringLiteral("0 B/s"), this);
    item.status = new QLabel(this);
    setRowStatus(item, QStringLiteral("Waiting"));

    grid_->addWidget(item.topic, row, 0);
    grid_->addWidget(item.bytes, row, 1);
    grid_->addWidget(item.speed, row, 2);
    grid_->addWidget(item.status, row, 3);
    rows_.insert(topic, item);
    ++row;
  }

  summary_label_->clear();
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
    if (!row->finished)
    {
      const qint64 now = elapsed_.elapsed();
      addSpeedSample(row->speed_samples, now, row->network_bytes);
      addSpeedSample(total_speed_samples_, now, networkTotal());
      row->speed->setText(formatSpeed(rollingSpeed(row->speed_samples)));
    }
  }
  else
  {
    row->cache_bytes = bytes;
  }
  row->total_bytes = total_bytes;
  if (from_network && total_bytes > 0)
  {
    row->bytes->setText(
        QStringLiteral("%1 / %2").arg(formatBytes(bytes), formatBytes(total_bytes)));
  }
  else
  {
    row->bytes->setText(from_network ? formatBytes(bytes) :
                                       QStringLiteral("cache %1").arg(formatBytes(bytes)));
  }
  if (!row->finished)
  {
    setRowStatus(*row, from_network ? QStringLiteral("Downloading") : QStringLiteral("Cache hit"));
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
  row->speed_samples.clear();
  row->speed->setText(formatSpeed(0));
  setRowStatus(*row, display_status);
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
      setRowStatus(*it, QStringLiteral("Cancelling"));
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
  const qint64 now = elapsed_.elapsed();
  for (auto it = rows_.begin(); it != rows_.end(); ++it)
  {
    network_total += it->network_bytes;
    if (!it->finished)
    {
      addSpeedSample(it->speed_samples, now, it->network_bytes);
      it->speed->setText(formatSpeed(rollingSpeed(it->speed_samples)));
    }
  }

  addSpeedSample(total_speed_samples_, now, network_total);
  const qint64 total_speed = active_ ? rollingSpeed(total_speed_samples_) : 0;
  const QString state = cancelling_ ? QStringLiteral("Cancelling") :
                        active_     ? QStringLiteral("Downloading") :
                                      QStringLiteral("Complete");
  summary_label_->setText(
      QStringLiteral("%1 - downloaded %2, speed %3")
          .arg(state, formatByteCount(network_total), formatSpeed(total_speed)));
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

qint64 DownloadStatsDialog::networkTotal() const
{
  qint64 total = 0;
  for (auto it = rows_.constBegin(); it != rows_.constEnd(); ++it)
  {
    total += it->network_bytes;
  }
  return total;
}

void DownloadStatsDialog::clearRows()
{
  for (auto it = rows_.begin(); it != rows_.end(); ++it)
  {
    delete it->topic;
    delete it->bytes;
    delete it->speed;
    delete it->status;
  }
  rows_.clear();
}

void DownloadStatsDialog::setRowStatus(Row& row, const QString& status)
{
  row.status->setText(status);

  QString color = QStringLiteral("#57606a");
  if (status == QStringLiteral("Downloading"))
  {
    color = QStringLiteral("#0969da");
  }
  else if (status == QStringLiteral("Done"))
  {
    color = QStringLiteral("#1a7f37");
  }
  else if (status == QStringLiteral("Cached") || status == QStringLiteral("Cache hit"))
  {
    color = QStringLiteral("#0a7f83");
  }
  else if (status == QStringLiteral("Failed"))
  {
    color = QStringLiteral("#cf222e");
  }
  else if (status == QStringLiteral("Cancelling") || status == QStringLiteral("Cancelled"))
  {
    color = QStringLiteral("#9a6700");
  }

  row.status->setStyleSheet(QStringLiteral("color: %1; font-weight: 600;").arg(color));
}

void DownloadStatsDialog::addSpeedSample(QVector<SpeedSample>& samples, qint64 elapsed_ms,
                                         qint64 bytes)
{
  samples.push_back({ elapsed_ms, bytes });
  const qint64 oldest_kept = elapsed_ms - kSpeedWindowMs;
  while (samples.size() > 1 && samples.front().elapsed_ms < oldest_kept)
  {
    samples.removeFirst();
  }
}

qint64 DownloadStatsDialog::rollingSpeed(const QVector<SpeedSample>& samples)
{
  if (samples.size() < 2)
  {
    return 0;
  }

  const auto& first = samples.front();
  const auto& last = samples.back();
  const qint64 elapsed_ms = last.elapsed_ms - first.elapsed_ms;
  if (elapsed_ms <= 0)
  {
    return 0;
  }
  return std::max<qint64>(0, last.bytes - first.bytes) * 1000 / elapsed_ms;
}
