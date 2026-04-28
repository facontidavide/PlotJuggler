/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// SDK headers before Qt (arrow/flight/api.h vs Qt signals macro).
#include <flight/mosaico_client.hpp>

#include "fetch_worker.h"

#include <QDebug>
#include <QStringList>
#include <QThread>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace
{
// Wall-clock milliseconds since `start` — used to bracket SDK calls so we
// can correlate plugin-observed latency with the SDK-level per-RPC spans
// logged from mosaico_client.cpp.
inline qint64 elapsedMs(std::chrono::steady_clock::time_point start)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                               start)
      .count();
}
}  // namespace

FetchWorker::FetchWorker(QObject* parent) : QObject(parent)
{
  qRegisterMetaType<MosaicoClient*>("MosaicoClient*");
  qRegisterMetaType<SequenceInfo>("SequenceInfo");
  qRegisterMetaType<std::vector<SequenceInfo>>("std::vector<SequenceInfo>");
}

void FetchWorker::setClient(MosaicoClient* client)
{
  client_ = client;
}

void FetchWorker::requestCancel()
{
  cancel_flag_.store(true, std::memory_order_relaxed);
}

void FetchWorker::resetCancel()
{
  cancel_flag_.store(false, std::memory_order_relaxed);
}

bool FetchWorker::isCancelled() const
{
  return cancel_flag_.load(std::memory_order_relaxed);
}

// Every slot body runs on a background thread. An uncaught exception here
// would call std::terminate and kill PlotJuggler. Wrap bodies in try/catch
// and convert any exception into the slot's error signal so the worst
// outcome is a visible error in the plugin UI.

void FetchWorker::fetchSequences()
{
  try
  {
    if (!client_)
    {
      emit errorOccurred("Not connected");
      return;
    }
    qDebug() << "[Mosaico fetch] listSequences: start";
    const auto t0 = std::chrono::steady_clock::now();
    auto result = client_->listSequences(
        [this](const std::vector<SequenceInfo>& sequences) { emit sequenceListStarted(sequences); },
        [this](const SequenceInfo& sequence, int64_t completed, int64_t total) {
          emit sequenceInfoReady(sequence, static_cast<qint64>(completed),
                                 static_cast<qint64>(total));
        });
    const auto ms = elapsedMs(t0);
    if (!result.ok())
    {
      qWarning() << "[Mosaico fetch] listSequences: FAILED after" << ms << "ms —"
                 << QString::fromStdString(result.status().ToString());
      emit errorOccurred(QString::fromStdString(result.status().ToString()));
      return;
    }
    qDebug() << "[Mosaico fetch] listSequences: OK in" << ms << "ms,"
             << static_cast<int>(result->size()) << "sequences";
    emit sequencesReady(*result);
  }
  catch (const std::exception& e)
  {
    emit errorOccurred(QStringLiteral("listSequences failed: ") + QString::fromUtf8(e.what()));
  }
  catch (...)
  {
    emit errorOccurred("listSequences failed: unknown error");
  }
}

void FetchWorker::fetchTopics(const QString& sequence_name)
{
  try
  {
    if (!client_)
    {
      emit errorOccurred("Not connected");
      return;
    }
    qDebug() << "[Mosaico fetch] listTopics" << sequence_name << ": start";
    const auto t0 = std::chrono::steady_clock::now();
    auto result = client_->listTopics(sequence_name.toStdString());
    const auto ms = elapsedMs(t0);
    if (!result.ok())
    {
      if (result.status().IsNotImplemented())
      {
        qDebug() << "[Mosaico fetch] listTopics" << sequence_name << ": NotImplemented after" << ms
                 << "ms (server doesn't support it)";
        emit topicsReady(QStringList{}, std::vector<TopicInfo>{});
      }
      else
      {
        qWarning() << "[Mosaico fetch] listTopics" << sequence_name << ": FAILED after" << ms
                   << "ms —" << QString::fromStdString(result.status().ToString());
        emit errorOccurred(QString::fromStdString(result.status().ToString()));
      }
      return;
    }
    qDebug() << "[Mosaico fetch] listTopics" << sequence_name << ": OK in" << ms << "ms,"
             << static_cast<int>(result->size()) << "topics";
    QStringList names;
    for (const auto& info : *result)
    {
      names.append(QString::fromStdString(info.topic_name));
    }
    emit topicsReady(names, *result);
  }
  catch (const std::exception& e)
  {
    emit errorOccurred(QStringLiteral("listTopics failed: ") + QString::fromUtf8(e.what()));
  }
  catch (...)
  {
    emit errorOccurred("listTopics failed: unknown error");
  }
}

void FetchWorker::fetchTopicMetadata(const QString& sequence_name, const QString& topic_name)
{
  try
  {
    if (!client_)
    {
      // No connection — silently drop. The info pane already shows the
      // partial info from listTopics; nothing useful to surface here.
      return;
    }
    qDebug() << "[Mosaico fetch] getTopicMetadata" << sequence_name << "/" << topic_name
             << ": start";
    const auto t0 = std::chrono::steady_clock::now();
    auto result = client_->getTopicMetadata(sequence_name.toStdString(), topic_name.toStdString());
    const auto ms = elapsedMs(t0);
    if (!result.ok())
    {
      qDebug() << "[Mosaico fetch] getTopicMetadata" << sequence_name << "/" << topic_name
               << ": FAILED after" << ms << "ms —"
               << QString::fromStdString(result.status().ToString());
      return;
    }
    qDebug() << "[Mosaico fetch] getTopicMetadata" << sequence_name << "/" << topic_name
             << ": OK in" << ms << "ms";
    emit topicMetadataReady(sequence_name, topic_name, *result);
  }
  catch (...)
  {
    // Lazy info-pane fetch — never escalate to the user-visible error path.
  }
}

void FetchWorker::fetchDataMulti(const QString& sequence_name, const QStringList& topic_names,
                                 qint64 start_ns, qint64 end_ns)
{
  try
  {
    if (!client_)
    {
      // Surface one error per topic so pending_fetches_ drains cleanly.
      for (const auto& topic : topic_names)
      {
        emit topicErrorOccurred(topic, QStringLiteral("Not connected"));
      }
      return;
    }
    // Drain path: cancel raised before the SDK got the chance to start.
    // Surface one cancellation per topic so the plugin's pending counter
    // reaches zero without staging RPCs we'd immediately abort.
    if (cancel_flag_.load(std::memory_order_relaxed))
    {
      for (const auto& topic : topic_names)
      {
        emit topicErrorOccurred(topic, QStringLiteral("cancelled"));
      }
      return;
    }

    TimeRange range;
    if (start_ns > 0)
    {
      range.start_ns = start_ns;
    }
    if (end_ns > 0)
    {
      range.end_ns = end_ns;
    }

    std::vector<std::string> topics_std;
    topics_std.reserve(topic_names.size());
    for (const auto& t : topic_names)
    {
      topics_std.emplace_back(t.toStdString());
    }

    struct ProgressState
    {
      std::chrono::steady_clock::time_point last_emit{};
      bool ever_emitted = false;
    };
    std::mutex progress_mu;
    std::unordered_map<std::string, ProgressState> progress_state;
    for (const auto& t : topics_std)
    {
      progress_state.emplace(t, ProgressState{});
    }

    auto progress = [this, &progress_mu, &progress_state](const std::string& topic,
                                                          int64_t /*rows*/, int64_t bytes,
                                                          int64_t total_bytes) {
      bool emit_now = false;
      {
        std::lock_guard<std::mutex> lg(progress_mu);
        auto it = progress_state.find(topic);
        if (it == progress_state.end())
        {
          return;
        }
        auto& st = it->second;
        const auto now = std::chrono::steady_clock::now();
        if (!st.ever_emitted ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - st.last_emit).count() >=
                100)
        {
          st.last_emit = now;
          st.ever_emitted = true;
          emit_now = true;
        }
      }
      if (emit_now)
      {
        emit fetchProgress(QString::fromStdString(topic), static_cast<qint64>(bytes),
                           static_cast<qint64>(total_bytes));
      }
    };

    auto schema_cb = [this, sequence_name](const std::string& topic,
                                           const std::shared_ptr<arrow::Schema>& schema) {
      emit topicStreamStarted(sequence_name, QString::fromStdString(topic), schema);
    };

    auto batch_cb = [this, sequence_name](const std::string& topic,
                                          const std::shared_ptr<arrow::RecordBatch>& batch) {
      emit topicBatchReady(sequence_name, QString::fromStdString(topic), batch);
    };

    auto on_done = [this, sequence_name](const std::string& topic,
                                         arrow::Result<PullResult> result) {
      const QString topic_q = QString::fromStdString(topic);
      if (!result.ok())
      {
        qWarning() << "[Mosaico fetch] pullTopic" << sequence_name << "/" << topic_q << ": FAILED —"
                   << QString::fromStdString(result.status().ToString());
        emit topicErrorOccurred(topic_q, QString::fromStdString(result.status().ToString()));
        return;
      }
      emit topicStreamFinished(sequence_name, topic_q);
    };

    qDebug().noquote() << QString("[Mosaico fetch] pullTopics %1: dispatching %2 topics "
                                  "(worker_thread=%3), range=[%4, %5], topics=[%6]")
                              .arg(sequence_name)
                              .arg(topic_names.size())
                              .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                              .arg(start_ns)
                              .arg(end_ns)
                              .arg(topic_names.join(", "));
    const auto t0 = std::chrono::steady_clock::now();
    auto status = client_->pullTopics(sequence_name.toStdString(), topics_std, range, on_done,
                                      progress, &cancel_flag_, batch_cb, schema_cb,
                                      /*retain_batches=*/false);
    const auto ms = elapsedMs(t0);
    if (!status.ok())
    {
      // Dispatch-level failure (bad args, etc.) — pullTopics did NOT invoke
      // on_done for any topic, so we have to drain the pending counter
      // ourselves here.
      qWarning() << "[Mosaico fetch] pullTopics" << sequence_name << ": dispatch FAILED after" << ms
                 << "ms —" << QString::fromStdString(status.ToString());
      const QString msg = QString::fromStdString(status.ToString());
      for (const auto& topic : topic_names)
      {
        emit topicErrorOccurred(topic, msg);
      }
      return;
    }
    qDebug().noquote() << QString("[Mosaico fetch] pullTopics %1: done in %2 ms")
                              .arg(sequence_name)
                              .arg(ms);
  }
  catch (const std::exception& e)
  {
    const QString msg = QStringLiteral("pullTopics failed: ") + QString::fromUtf8(e.what());
    for (const auto& topic : topic_names)
    {
      emit topicErrorOccurred(topic, msg);
    }
  }
  catch (...)
  {
    for (const auto& topic : topic_names)
    {
      emit topicErrorOccurred(topic, QStringLiteral("pullTopics failed: unknown error"));
    }
  }
}
