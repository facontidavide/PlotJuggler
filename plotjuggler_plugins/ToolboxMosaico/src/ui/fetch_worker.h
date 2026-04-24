/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flight/types.hpp>
using mosaico::PullResult;
using mosaico::SequenceInfo;
using mosaico::TimeRange;
using mosaico::TopicInfo;

#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <vector>

namespace mosaico
{
class MosaicoClient;
}
using mosaico::MosaicoClient;

// Thin adapter living on a background QThread.
// Forwards calls to MosaicoClient and emits results as Qt signals.
class FetchWorker : public QObject
{
  Q_OBJECT

public:
  explicit FetchWorker(QObject* parent = nullptr);

  // Cancellation hooks. These are intentionally NOT slots — they're called
  // directly from the owning thread (typically the GUI thread) so the flag
  // is observed by an in-flight pullTopic on the worker thread without
  // needing the worker's event loop to drain.
  void requestCancel();
  void resetCancel();
  bool isCancelled() const;

public slots:
  void setClient(MosaicoClient* client);
  void fetchSequences();
  void fetchTopics(const QString& sequence_name);
  // Fetches the schema/ontology/user_metadata for a single topic. Used to
  // populate the info pane lazily after the user selects a topic — keeping
  // the per-topic GetFlightInfo round-trips off the open-sequence path.
  void fetchTopicMetadata(const QString& sequence_name, const QString& topic_name);
  // Fetches several topics in parallel via the SDK's connection pool. Each
  // topic emits its own dataReady or errorOccurred when it completes, so the
  // existing per-topic plugin bookkeeping (pending_fetches_, status updates)
  // keeps working unchanged. Cancel is honored mid-stream by every in-flight
  // pullTopic worker.
  void fetchDataMulti(const QString& sequence_name, const QStringList& topic_names, qint64 start_ns,
                      qint64 end_ns);

signals:
  void sequencesReady(const std::vector<SequenceInfo>& sequences);
  void topicsReady(const QStringList& names, const std::vector<TopicInfo>& infos);
  // Emitted after fetchTopicMetadata completes successfully. Failures are
  // swallowed (the info pane already shows the partial info from listTopics).
  void topicMetadataReady(const QString& sequence_name, const QString& topic_name,
                          const TopicInfo& info);
  void dataReady(const QString& sequence_name, const QString& topic_name, const PullResult& result);
  // Emitted periodically during a pull. bytes = cumulative bytes received,
  // total_bytes = server-advertised total (0 when unknown).
  void fetchProgress(const QString& topic_name, qint64 bytes, qint64 total_bytes);
  void errorOccurred(const QString& message);

private:
  MosaicoClient* client_ = nullptr;
  // Set by the GUI thread via requestCancel(); read by pullTopic in the
  // worker thread between chunks (and at fetchDataMulti entry so a cancel
  // raised before the SDK starts is honored without a wasted dispatch).
  std::atomic<bool> cancel_flag_{ false };
};
