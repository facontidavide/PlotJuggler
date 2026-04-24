#pragma once

#include <QHash>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>

class QThread;

class UploadWorker;

struct UploadRequest
{
  QString file_path;
  QString sequence_name;
};

// Orchestrates up to kMaxConcurrent active UploadWorker threads.
// Extra requests queue and start as slots free.
class UploadManager : public QObject
{
  Q_OBJECT

public:
  static constexpr int kMaxConcurrent = 4;

  explicit UploadManager(QObject* parent = nullptr);
  ~UploadManager() override;

  // Set the connection parameters used by future workers.
  // Existing in-flight workers continue with their original parameters.
  void setConnection(const QString& uri, const QString& cert_path,
                     const QString& api_key);

  // Enqueue N files for upload. Sequence names must be pre-resolved.
  void enqueue(const QList<UploadRequest>& requests);

  // Names currently in the queue or in-flight (not yet succeeded/failed).
  [[nodiscard]] QStringList activeNames() const;

  // Abort all in-flight uploads. Emits failed(..., "cancelled") for each.
  void shutdown();

signals:
  void uploadStarted(const QString& sequence_name);
  void uploadProgress(const QString& sequence_name, int percent);
  void uploadSucceeded(const QString& sequence_name);
  void uploadFailed(const QString& sequence_name, const QString& message);

private slots:
  void onWorkerSucceeded(const QString& sequence_name);
  void onWorkerFailed(const QString& sequence_name, const QString& message);

private:
  void tryStartNext();
  void cleanupWorker(const QString& sequence_name);

  QString uri_;
  QString cert_path_;
  QString api_key_;

  QQueue<UploadRequest> pending_;
  QHash<QString, UploadWorker*> workers_;   // keyed by sequence_name
  QHash<QString, QThread*> threads_;
};
