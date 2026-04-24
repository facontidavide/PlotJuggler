#include "upload_manager.h"
#include "upload_worker.h"

#include <QDebug>
#include <QThread>

// Upper bound on how long we wait for a worker thread to notice its
// cancel flag during shutdown. uploadMcap() polls the flag at channel
// boundaries and inside decode callbacks, so a long DoPut stream can
// keep a thread busy for a few seconds before it exits. If this bound
// is exceeded the thread is terminated rather than blocking app close.
static constexpr int kShutdownWaitMs = 5000;

UploadManager::UploadManager(QObject* parent) : QObject(parent) {}

UploadManager::~UploadManager()
{
  shutdown();
}

void UploadManager::setConnection(const QString& uri,
                                  const QString& cert_path,
                                  const QString& api_key)
{
  uri_ = uri;
  cert_path_ = cert_path;
  api_key_ = api_key;
}

void UploadManager::enqueue(const QList<UploadRequest>& requests)
{
  for (const auto& req : requests) pending_.enqueue(req);
  for (int i = 0; i < kMaxConcurrent; ++i) tryStartNext();
}

QStringList UploadManager::activeNames() const
{
  QStringList names;
  for (const auto& req : pending_) names << req.sequence_name;
  for (auto it = workers_.begin(); it != workers_.end(); ++it)
    names << it.key();
  return names;
}

void UploadManager::shutdown()
{
  pending_.clear();
  for (auto* worker : workers_) worker->cancel();
  for (auto* thread : threads_)
  {
    thread->quit();
    if (!thread->wait(kShutdownWaitMs))
    {
      qWarning() << "[Mosaico upload] worker did not finish within"
                 << kShutdownWaitMs << "ms — terminating";
      thread->terminate();
      thread->wait();
    }
  }
  for (auto* worker : workers_) delete worker;
  workers_.clear();
  for (auto* thread : threads_) delete thread;
  threads_.clear();
}

void UploadManager::tryStartNext()
{
  if (pending_.isEmpty()) return;
  if (workers_.size() >= kMaxConcurrent) return;

  auto req = pending_.dequeue();
  auto* worker = new UploadWorker(req.file_path, req.sequence_name,
                                   uri_, cert_path_, api_key_);
  auto* thread = new QThread(this);
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &UploadWorker::run);
  connect(worker, &UploadWorker::progress, this,
          &UploadManager::uploadProgress);
  connect(worker, &UploadWorker::succeeded, this,
          &UploadManager::onWorkerSucceeded);
  connect(worker, &UploadWorker::failed, this,
          &UploadManager::onWorkerFailed);

  workers_.insert(req.sequence_name, worker);
  threads_.insert(req.sequence_name, thread);

  emit uploadStarted(req.sequence_name);
  thread->start();
}

void UploadManager::onWorkerSucceeded(const QString& sequence_name)
{
  emit uploadSucceeded(sequence_name);
  cleanupWorker(sequence_name);
  tryStartNext();
}

void UploadManager::onWorkerFailed(const QString& sequence_name,
                                    const QString& message)
{
  emit uploadFailed(sequence_name, message);
  cleanupWorker(sequence_name);
  tryStartNext();
}

void UploadManager::cleanupWorker(const QString& sequence_name)
{
  auto worker_it = workers_.find(sequence_name);
  auto thread_it = threads_.find(sequence_name);
  if (thread_it != threads_.end())
  {
    thread_it.value()->quit();
    thread_it.value()->wait();
    delete thread_it.value();
    threads_.erase(thread_it);
  }
  if (worker_it != workers_.end())
  {
    delete worker_it.value();
    workers_.erase(worker_it);
  }
}
