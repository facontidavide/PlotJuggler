#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <memory>

namespace mosaico { class MosaicoClient; }
using mosaico::MosaicoClient;

// Runs a single MCAP upload on its own QThread. Owns the MosaicoClient.
// Emits progress and completion signals via queued connections.
class UploadWorker : public QObject
{
  Q_OBJECT

public:
  UploadWorker(QString file_path, QString sequence_name,
               QString server_uri, QString tls_cert_path, QString api_key,
               QObject* parent = nullptr);
  ~UploadWorker() override;

  [[nodiscard]] QString sequenceName() const { return sequence_name_; }

  // Request a graceful abort. The worker will clean up any partial upload.
  void cancel();

public slots:
  void run();

signals:
  void progress(const QString& sequence_name, int percent);
  void succeeded(const QString& sequence_name);
  void failed(const QString& sequence_name, const QString& message);

private:
  QString file_path_;
  QString sequence_name_;
  QString server_uri_;
  QString tls_cert_path_;
  QString api_key_;
  std::atomic<bool> cancel_{false};
  std::unique_ptr<MosaicoClient> client_;
};
