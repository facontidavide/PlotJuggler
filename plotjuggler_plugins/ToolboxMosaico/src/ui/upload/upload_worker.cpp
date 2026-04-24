// Arrow/SDK headers MUST come before Qt — Arrow Flight pulls in gRPC,
// which uses `signals` as a parameter name, conflicting with Qt's macro.
#include <flight/mosaico_client.hpp>
#include <flight/upload_mcap.hpp>
using mosaico::uploadMcap;
using mosaico::UploadReport;

#include "upload_worker.h"

#include <QDebug>

// Uploads use a separate, longer timeout and a single-connection pool
// because DoPut streams can run much longer than a discovery RPC, and
// each UploadWorker owns its own client/thread — no concurrency to share.
// The interactive client in MainWindow (30 s / pool=4) is tuned for
// lightweight list/pull RPCs and would time out long uploads.
static constexpr int kUploadTimeoutSec = 60;
static constexpr int kUploadPoolSize = 1;

UploadWorker::UploadWorker(QString file_path, QString sequence_name,
                            QString server_uri, QString tls_cert_path,
                            QString api_key, QObject* parent)
  : QObject(parent),
    file_path_(std::move(file_path)),
    sequence_name_(std::move(sequence_name)),
    server_uri_(std::move(server_uri)),
    tls_cert_path_(std::move(tls_cert_path)),
    api_key_(std::move(api_key))
{}

UploadWorker::~UploadWorker() = default;

void UploadWorker::cancel()
{
  cancel_.store(true, std::memory_order_relaxed);
}

void UploadWorker::run()
{
  // Runs on the UploadManager's dedicated thread. An uncaught exception
  // here would call std::terminate and take PlotJuggler down with it;
  // the catch-all below is the worker's last line of defense.
  try
  {
    qDebug() << "[Mosaico upload]" << sequence_name_
             << "starting — uri=" << server_uri_
             << "cert=" << tls_cert_path_
             << "file=" << file_path_;

    client_ = std::make_unique<MosaicoClient>(
        server_uri_.toStdString(), /*timeout=*/kUploadTimeoutSec, /*pool_size=*/kUploadPoolSize,
        tls_cert_path_.toStdString(), api_key_.toStdString());

    auto seq_name = sequence_name_;
    std::string last_topic;
    int64_t last_rows = 0;

    auto progress_cb = [&, seq_name](int64_t rows, int64_t /*bytes*/,
                                     int64_t total, const std::string& topic) {
      if (topic != last_topic) {
        qDebug() << "[Mosaico upload]" << seq_name << "topic:"
                 << QString::fromStdString(topic) << "total=" << total;
        last_topic = topic;
      }
      last_rows = rows;
      if (total <= 0) return;
      int percent = static_cast<int>((rows * 100) / total);
      if (percent > 100) percent = 100;
      emit progress(seq_name, percent);
    };

    UploadReport report;
    auto status = uploadMcap(
        *client_, file_path_.toStdString(), sequence_name_.toStdString(),
        progress_cb, &cancel_, &report);

    if (status.ok())
    {
      qDebug() << "[Mosaico upload]" << sequence_name_ << "OK —"
               << report.topics_succeeded << "topics";
      emit succeeded(sequence_name_);
      return;
    }

    // qWarning() is appropriate here — failure is a real event the operator
    // should notice in logs. Successes stay on qDebug().
    qWarning() << "[Mosaico upload]" << sequence_name_ << "FAILED:"
               << QString::fromStdString(status.ToString())
               << "— succeeded=" << report.topics_succeeded
               << "failed=" << static_cast<int>(report.topic_failures.size())
               << "last_topic=" << QString::fromStdString(last_topic)
               << "last_rows=" << last_rows;

    emit failed(sequence_name_, QString::fromStdString(status.ToString()));
  }
  catch (const std::exception& e)
  {
    qWarning() << "[Mosaico upload]" << sequence_name_
               << "FAILED (exception):" << e.what();
    emit failed(sequence_name_,
                QStringLiteral("unexpected error: ") +
                    QString::fromUtf8(e.what()));
  }
  catch (...)
  {
    qWarning() << "[Mosaico upload]" << sequence_name_
               << "FAILED (unknown exception)";
    emit failed(sequence_name_, "unexpected error (unknown)");
  }
}
