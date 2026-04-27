/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// SDK headers before Qt (arrow/flight/api.h vs Qt signals macro).
#include <flight/mosaico_client.hpp>

#include "fetch_worker.h"

#include <arrow/util/byte_size.h>

#include <QDebug>
#include <QStringList>
#include <QThread>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

mosaico_cache::CacheKey makeCacheKey(const QString& cache_namespace, const QString& sequence_name,
                                     const QString& topic_name, qint64 start_ns, qint64 end_ns)
{
  mosaico_cache::CacheKey key;
  const QString ns =
      cache_namespace.isEmpty() ? QStringLiteral("unknown-server") : cache_namespace;
  key.dataset_id = ns + QStringLiteral("/") + sequence_name;
  key.dataset_version = QStringLiteral("unversioned");
  key.topic = topic_name;
  key.chunk_start_ns = start_ns;
  key.chunk_end_ns = end_ns;
  return key;
}
}  // namespace

FetchWorker::FetchWorker(QObject* parent) : QObject(parent)
{
  qRegisterMetaType<MosaicoClient*>("MosaicoClient*");
}

void FetchWorker::setClient(MosaicoClient* client)
{
  client_ = client;
}

void FetchWorker::setCacheNamespace(const QString& namespace_key)
{
  cache_namespace_ = namespace_key;
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
    auto result = client_->listSequences();
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

    const QString cache_ns = cache_namespace_;
    QStringList miss_topics;
    miss_topics.reserve(topic_names.size());
    for (const auto& topic : topic_names)
    {
      auto key = makeCacheKey(cache_ns, sequence_name, topic, start_ns, end_ns);
      qint64 cached_bytes = 0;
      bool cache_cancelled = false;
      auto cached = cache_.readStreaming(
          key,
          [this, sequence_name, topic](const std::shared_ptr<arrow::Schema>& schema) {
            emit topicStreamStarted(sequence_name, topic, schema);
          },
          [this, sequence_name, topic, &cached_bytes, &cache_cancelled](
              const std::shared_ptr<arrow::RecordBatch>& batch) {
            if (cancel_flag_.load(std::memory_order_relaxed))
            {
              cache_cancelled = true;
              return false;
            }
            if (batch)
            {
              cached_bytes += static_cast<qint64>(arrow::util::TotalBufferSize(*batch));
            }
            emit topicBatchReady(sequence_name, topic, batch);
            emit fetchProgress(topic, cached_bytes, 0, false);
            return true;
          });
      if (cache_cancelled)
      {
        emit topicErrorOccurred(topic, QStringLiteral("cancelled"));
        continue;
      }
      if (cached.hit)
      {
        qDebug().noquote() << QString("[Mosaico cache] HIT %1/%2 range=[%3, %4]")
                                  .arg(sequence_name)
                                  .arg(topic)
                                  .arg(start_ns)
                                  .arg(end_ns);
        emit topicStreamFinished(sequence_name, topic);
        continue;
      }
      if (!cached.error.isEmpty())
      {
        qDebug().noquote() << QString("[Mosaico cache] lookup skipped for %1/%2: %3")
                                  .arg(sequence_name, topic, cached.error);
      }
      miss_topics.append(topic);
    }

    if (miss_topics.isEmpty())
    {
      qDebug().noquote() << QString("[Mosaico cache] all %1 topic(s) served from cache")
                                .arg(topic_names.size());
      return;
    }

    std::vector<std::string> topics_std;
    topics_std.reserve(miss_topics.size());
    for (const auto& t : miss_topics)
    {
      topics_std.emplace_back(t.toStdString());
    }

    // Per-topic throttle: pullTopics fires the progress callback from
    // multiple worker threads concurrently, so each topic owns its own
    // last_emit timestamp under a single mutex. A typical pull bursts a few
    // hundred chunks/sec; capping at ~10 emits/sec/topic keeps the GUI
    // event loop relaxed while still feeling responsive.
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
        const bool done = total_bytes > 0 && bytes >= total_bytes;
        if (!st.ever_emitted || done ||
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
                           static_cast<qint64>(total_bytes), true);
      }
    };

    QString stream_cache_open_error;
    const bool stream_cache_enabled = cache_.open(&stream_cache_open_error);
    if (!stream_cache_enabled && !stream_cache_open_error.isEmpty())
    {
      qDebug().noquote() << QString("[Mosaico cache] stream store disabled: %1")
                                .arg(stream_cache_open_error);
    }

    std::mutex cache_writer_mu;
    std::unordered_map<std::string, std::shared_ptr<mosaico_cache::LocalCache::Writer>>
        cache_writers;
    std::unordered_map<std::string, QString> cache_stream_errors;
    auto note_cache_stream_error =
        [&cache_writer_mu, &cache_stream_errors](const std::string& topic, const QString& error) {
          if (error.isEmpty())
          {
            return;
          }
          std::lock_guard<std::mutex> lg(cache_writer_mu);
          if (cache_stream_errors.find(topic) == cache_stream_errors.end())
          {
            cache_stream_errors.emplace(topic, error);
          }
        };

    auto cache_schema =
        [this, cache_ns, sequence_name, start_ns, end_ns, stream_cache_enabled,
         &cache_writer_mu, &cache_writers, &note_cache_stream_error](
            const std::string& topic, const std::shared_ptr<arrow::Schema>& schema) {
          if (!stream_cache_enabled)
          {
            emit topicStreamStarted(sequence_name, QString::fromStdString(topic), schema);
            return;
          }
          const QString topic_q = QString::fromStdString(topic);
          emit topicStreamStarted(sequence_name, topic_q, schema);
          const auto key = makeCacheKey(cache_ns, sequence_name, topic_q, start_ns, end_ns);
          QString cache_error;
          auto pending = cache_.beginWrite(key, schema, &cache_error);
          if (!pending.isOpen())
          {
            note_cache_stream_error(topic, cache_error);
            return;
          }

          auto writer =
              std::make_shared<mosaico_cache::LocalCache::Writer>(std::move(pending));
          std::lock_guard<std::mutex> lg(cache_writer_mu);
          cache_writers[topic] = std::move(writer);
        };

    auto cache_batch =
        [this, sequence_name, &cache_writer_mu, &cache_writers, &note_cache_stream_error](
            const std::string& topic, const std::shared_ptr<arrow::RecordBatch>& batch) {
          std::shared_ptr<mosaico_cache::LocalCache::Writer> writer;
          {
            std::lock_guard<std::mutex> lg(cache_writer_mu);
            auto it = cache_writers.find(topic);
            if (it != cache_writers.end())
            {
              writer = it->second;
            }
          }

          if (writer)
          {
            QString cache_error;
            if (!writer->writeBatch(batch, &cache_error))
            {
              note_cache_stream_error(topic, cache_error);
            }
          }
          emit topicBatchReady(sequence_name, QString::fromStdString(topic), batch);
        };

    auto take_cache_writer =
        [&cache_writer_mu, &cache_writers, &cache_stream_errors](
            const std::string& topic, QString* stream_error) {
          std::shared_ptr<mosaico_cache::LocalCache::Writer> writer;
          std::lock_guard<std::mutex> lg(cache_writer_mu);
          auto writer_it = cache_writers.find(topic);
          if (writer_it != cache_writers.end())
          {
            writer = writer_it->second;
            cache_writers.erase(writer_it);
          }
          auto error_it = cache_stream_errors.find(topic);
          if (error_it != cache_stream_errors.end())
          {
            if (stream_error)
            {
              *stream_error = error_it->second;
            }
            cache_stream_errors.erase(error_it);
          }
          return writer;
        };

    auto on_done = [this, sequence_name, start_ns, end_ns, &take_cache_writer](
                       const std::string& topic, arrow::Result<PullResult> result) {
      const QString topic_q = QString::fromStdString(topic);
      if (!result.ok())
      {
        QString stream_error;
        if (auto writer = take_cache_writer(topic, &stream_error))
        {
          writer->abort();
        }
        qWarning() << "[Mosaico fetch] pullTopic" << sequence_name << "/" << topic_q << ": FAILED —"
                   << QString::fromStdString(result.status().ToString());
        emit topicErrorOccurred(topic_q, QString::fromStdString(result.status().ToString()));
        return;
      }

      QString cache_error;
      QString stream_error;
      auto writer = take_cache_writer(topic, &stream_error);
      if (!stream_error.isEmpty())
      {
        if (writer)
        {
          writer->abort();
        }
        qDebug().noquote() << QString("[Mosaico cache] stream store skipped for %1/%2: %3")
                                  .arg(sequence_name, topic_q, stream_error);
      }
      else if (writer && writer->commit(&cache_error))
      {
        qDebug().noquote() << QString("[Mosaico cache] stored %1/%2 range=[%3, %4]")
                                  .arg(sequence_name)
                                  .arg(topic_q)
                                  .arg(start_ns)
                                  .arg(end_ns);
      }
      else if (!cache_error.isEmpty())
      {
        qDebug().noquote() << QString("[Mosaico cache] store skipped for %1/%2: %3")
                                  .arg(sequence_name, topic_q, cache_error);
      }

      emit topicStreamFinished(sequence_name, topic_q);
    };

    qDebug().noquote() << QString("[Mosaico fetch] pullTopics %1: dispatching %2 topics "
                                  "(%3 cache miss, worker_thread=%4), range=[%5, %6], topics=[%7]")
                              .arg(sequence_name)
                              .arg(topic_names.size())
                              .arg(miss_topics.size())
                              .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                              .arg(start_ns)
                              .arg(end_ns)
                              .arg(miss_topics.join(", "));
    const auto t0 = std::chrono::steady_clock::now();
    auto status = client_->pullTopics(sequence_name.toStdString(), topics_std, range, on_done,
                                      progress, &cancel_flag_, cache_batch, cache_schema,
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
      for (const auto& topic : miss_topics)
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
