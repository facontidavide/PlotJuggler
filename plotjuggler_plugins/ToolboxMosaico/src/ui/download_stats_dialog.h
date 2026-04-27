/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QDialog>
#include <QElapsedTimer>
#include <QHash>
#include <QString>
#include <QStringList>

class QGridLayout;
class QLabel;
class QPushButton;
class QProgressBar;
class QTimer;

class DownloadStatsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit DownloadStatsDialog(QWidget* parent = nullptr);

  void start(const QStringList& topics);
  void updateProgress(const QString& topic, qint64 bytes, qint64 total_bytes, bool from_network);
  void markFinished(const QString& topic, const QString& status);
  void markCancelling();
  void markComplete();
  void reject() override;

signals:
  void cancelRequested();

private slots:
  void refreshStats();

private:
  struct Row
  {
    QLabel* topic = nullptr;
    QProgressBar* progress = nullptr;
    QLabel* bytes = nullptr;
    QLabel* speed = nullptr;
    QLabel* status = nullptr;
    qint64 network_bytes = 0;
    qint64 previous_network_bytes = 0;
    qint64 cache_bytes = 0;
    qint64 total_bytes = 0;
    bool finished = false;
  };

  Row* rowForTopic(const QString& topic);
  void clearRows();

  QLabel* summary_label_ = nullptr;
  QGridLayout* grid_ = nullptr;
  QPushButton* cancel_button_ = nullptr;
  QTimer* timer_ = nullptr;
  QElapsedTimer elapsed_;
  QHash<QString, Row> rows_;
  qint64 previous_total_bytes_ = 0;
  bool active_ = false;
  bool cancelling_ = false;
};
