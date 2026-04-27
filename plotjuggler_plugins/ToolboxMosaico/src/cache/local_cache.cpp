/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "cache/local_cache.h"

#include <arrow/io/file.h>
#include <arrow/ipc/options.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/writer.h>
#include <arrow/record_batch.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type.h>
#include <arrow/util/byte_size.h>
#include <arrow/util/compression.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUuid>
#include <QVariant>

#include <memory>
#include <utility>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace mosaico_cache
{
namespace
{

QString sqlError(const QSqlQuery& query)
{
  return query.lastError().text();
}

QString sqlError(const QSqlDatabase& db)
{
  return db.lastError().text();
}

QString hexHash(const QString& text)
{
  return QString::fromLatin1(
      QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString versionDir(const QString& version)
{
  if (version.isEmpty())
  {
    return QStringLiteral("unversioned");
  }
  return hexHash(version);
}

std::shared_ptr<arrow::Schema> schemaFor(const PullResult& result)
{
  if (result.schema)
  {
    return result.schema;
  }
  for (const auto& batch : result.batches)
  {
    if (batch)
    {
      return batch->schema();
    }
  }
  return {};
}

QString statusToQString(const arrow::Status& status)
{
  return QString::fromStdString(status.ToString());
}

arrow::Result<arrow::ipc::IpcWriteOptions> makeWriteOptions()
{
  auto options = arrow::ipc::IpcWriteOptions::Defaults();
  constexpr auto codec = arrow::Compression::LZ4_FRAME;
  if (!arrow::util::Codec::IsAvailable(codec))
  {
    return arrow::Status::NotImplemented("Arrow LZ4_FRAME codec is not available");
  }

  auto codec_result = arrow::util::Codec::Create(codec);
  if (!codec_result.ok())
  {
    return codec_result.status();
  }
  options.codec = std::shared_ptr<arrow::util::Codec>(std::move(*codec_result));
  options.min_space_savings = 0.01;
  return options;
}

}  // namespace

LocalCache::LocalCache(QString root_path, qint64 max_size_bytes)
  : root_path_(std::move(root_path))
  , max_size_bytes_(max_size_bytes)
  , connection_name_(QStringLiteral("ToolboxMosaicoCache_%1")
                         .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

LocalCache::~LocalCache()
{
  if (!connection_name_.isEmpty() && QSqlDatabase::contains(connection_name_))
  {
    {
      QSqlDatabase db = QSqlDatabase::database(connection_name_, false);
      if (db.isValid())
      {
        db.close();
      }
    }
    QSqlDatabase::removeDatabase(connection_name_);
  }
}

QString LocalCache::defaultRootPath()
{
  QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  if (base.isEmpty())
  {
    base = QDir::homePath() + QStringLiteral("/.cache/PlotJuggler");
  }
  return QDir(base).filePath(QStringLiteral("mosaico"));
}

qint64 LocalCache::defaultBudget()
{
  return 5LL * 1024LL * 1024LL * 1024LL;
}

bool LocalCache::open(QString* error)
{
  return ensureOpen(error);
}

bool LocalCache::ensureOpen(QString* error)
{
  if (opened_)
  {
    return true;
  }

  QDir root(root_path_);
  if (!root.mkpath(QStringLiteral(".")))
  {
    if (error)
    {
      *error = QStringLiteral("cannot create cache root: %1").arg(root_path_);
    }
    return false;
  }

  if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")))
  {
    if (error)
    {
      *error = QStringLiteral("Qt SQLite driver is not available");
    }
    return false;
  }

  QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection_name_);
  db.setDatabaseName(root.filePath(QStringLiteral("index.sqlite")));
  if (!db.open())
  {
    if (error)
    {
      *error = QStringLiteral("cannot open cache index: %1").arg(sqlError(db));
    }
    return false;
  }

  if (!execPragma(QStringLiteral("PRAGMA journal_mode = WAL"), error))
  {
    return false;
  }
  if (!execPragma(QStringLiteral("PRAGMA synchronous = NORMAL"), error))
  {
    return false;
  }
  if (!ensureSchema(error))
  {
    return false;
  }

  opened_ = true;
  return true;
}

QSqlDatabase LocalCache::database() const
{
  return QSqlDatabase::database(connection_name_);
}

bool LocalCache::execPragma(const QString& sql, QString* error)
{
  QSqlQuery q(database());
  if (!q.exec(sql))
  {
    if (error)
    {
      *error = QStringLiteral("%1 failed: %2").arg(sql, sqlError(q));
    }
    return false;
  }
  return true;
}

bool LocalCache::ensureSchema(QString* error)
{
  QSqlQuery q(database());
  const char* schema_sql =
      "CREATE TABLE IF NOT EXISTS chunks ("
      "dataset_id TEXT NOT NULL,"
      "dataset_version TEXT NOT NULL,"
      "topic TEXT NOT NULL,"
      "chunk_start INTEGER NOT NULL,"
      "chunk_end INTEGER NOT NULL,"
      "path TEXT NOT NULL,"
      "size_bytes INTEGER NOT NULL,"
      "last_access INTEGER NOT NULL,"
      "ticket BLOB,"
      "PRIMARY KEY (dataset_id, dataset_version, topic, chunk_start, chunk_end)"
      ")";
  if (!q.exec(QString::fromLatin1(schema_sql)))
  {
    if (error)
    {
      *error = QStringLiteral("cannot create cache schema: %1").arg(sqlError(q));
    }
    return false;
  }
  if (!q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_lru ON chunks(last_access)")))
  {
    if (error)
    {
      *error = QStringLiteral("cannot create LRU index: %1").arg(sqlError(q));
    }
    return false;
  }
  if (!q.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_range ON chunks("
                             "dataset_id, dataset_version, topic, chunk_start)")))
  {
    if (error)
    {
      *error = QStringLiteral("cannot create range index: %1").arg(sqlError(q));
    }
    return false;
  }
  return true;
}

QString LocalCache::relativePathForKey(const CacheKey& key) const
{
  const QString topic_hash = hexHash(key.dataset_id + QStringLiteral("|") + key.topic);
  const QString dataset_hash = hexHash(key.dataset_id);
  const QString version_hash = versionDir(key.dataset_version);
  const QString file_name =
      QStringLiteral("%1_%2.arrow").arg(key.chunk_start_ns).arg(key.chunk_end_ns);
  return QDir(QStringLiteral("data"))
      .filePath(topic_hash.left(2) + QStringLiteral("/") + topic_hash + QStringLiteral("/") +
                dataset_hash + QStringLiteral("/") + version_hash + QStringLiteral("/") +
                file_name);
}

QString LocalCache::absolutePath(const QString& relative_path) const
{
  return QDir(root_path_).filePath(relative_path);
}

qint64 LocalCache::currentAccessTime() const
{
  return QDateTime::currentMSecsSinceEpoch();
}

CacheReadResult LocalCache::read(const CacheKey& key)
{
  CacheReadResult out;
  QString error;
  if (!ensureOpen(&error))
  {
    out.error = error;
    return out;
  }

  QSqlQuery q(database());
  q.prepare(QStringLiteral("SELECT path FROM chunks "
                           "WHERE dataset_id=? AND dataset_version=? AND topic=? "
                           "AND chunk_start=? AND chunk_end=?"));
  q.addBindValue(key.dataset_id);
  q.addBindValue(key.dataset_version);
  q.addBindValue(key.topic);
  q.addBindValue(key.chunk_start_ns);
  q.addBindValue(key.chunk_end_ns);
  if (!q.exec())
  {
    out.error = QStringLiteral("cache lookup failed: %1").arg(sqlError(q));
    return out;
  }
  if (!q.next())
  {
    return out;
  }

  const QString rel_path = q.value(0).toString();
  const QString abs_path = absolutePath(rel_path);
  if (!QFileInfo::exists(abs_path))
  {
    removeEntry(key, nullptr);
    return out;
  }

  auto file_result = arrow::io::MemoryMappedFile::Open(abs_path.toStdString(),
                                                       arrow::io::FileMode::READ);
  if (!file_result.ok())
  {
    out.error = statusToQString(file_result.status());
    return out;
  }

  auto reader_result = arrow::ipc::RecordBatchFileReader::Open(*file_result);
  if (!reader_result.ok())
  {
    out.error = statusToQString(reader_result.status());
    return out;
  }
  auto reader = *reader_result;

  PullResult result;
  result.schema = reader->schema();
  result.batches.reserve(reader->num_record_batches());
  for (int i = 0; i < reader->num_record_batches(); ++i)
  {
    auto batch_result = reader->ReadRecordBatch(i);
    if (!batch_result.ok())
    {
      out.error = statusToQString(batch_result.status());
      return out;
    }
    result.batches.push_back(*batch_result);
  }

  updateLastAccess(key, currentAccessTime(), nullptr);
  out.hit = true;
  out.result = std::move(result);
  return out;
}

CacheStreamResult LocalCache::readStreaming(
    const CacheKey& key,
    const std::function<void(const std::shared_ptr<arrow::Schema>&)>& schema_cb,
    const std::function<bool(const std::shared_ptr<arrow::RecordBatch>&)>& batch_cb)
{
  CacheStreamResult out;
  QString error;
  if (!ensureOpen(&error))
  {
    out.error = error;
    return out;
  }

  QSqlQuery q(database());
  q.prepare(QStringLiteral("SELECT path FROM chunks "
                           "WHERE dataset_id=? AND dataset_version=? AND topic=? "
                           "AND chunk_start=? AND chunk_end=?"));
  q.addBindValue(key.dataset_id);
  q.addBindValue(key.dataset_version);
  q.addBindValue(key.topic);
  q.addBindValue(key.chunk_start_ns);
  q.addBindValue(key.chunk_end_ns);
  if (!q.exec())
  {
    out.error = QStringLiteral("cache lookup failed: %1").arg(sqlError(q));
    return out;
  }
  if (!q.next())
  {
    return out;
  }

  const QString rel_path = q.value(0).toString();
  const QString abs_path = absolutePath(rel_path);
  if (!QFileInfo::exists(abs_path))
  {
    removeEntry(key, nullptr);
    return out;
  }

  auto file_result = arrow::io::MemoryMappedFile::Open(abs_path.toStdString(),
                                                       arrow::io::FileMode::READ);
  if (!file_result.ok())
  {
    out.error = statusToQString(file_result.status());
    return out;
  }

  auto reader_result = arrow::ipc::RecordBatchFileReader::Open(*file_result);
  if (!reader_result.ok())
  {
    out.error = statusToQString(reader_result.status());
    return out;
  }
  auto reader = *reader_result;

  if (schema_cb)
  {
    schema_cb(reader->schema());
  }

  for (int i = 0; i < reader->num_record_batches(); ++i)
  {
    auto batch_result = reader->ReadRecordBatch(i);
    if (!batch_result.ok())
    {
      out.error = statusToQString(batch_result.status());
      return out;
    }
    auto batch = *batch_result;
    if (batch)
    {
      out.decoded_bytes += static_cast<qint64>(arrow::util::TotalBufferSize(*batch));
    }
    if (batch_cb && !batch_cb(batch))
    {
      out.error = QStringLiteral("cache read cancelled");
      return out;
    }
  }

  updateLastAccess(key, currentAccessTime(), nullptr);
  out.hit = true;
  return out;
}

struct LocalCache::Writer::Impl
{
  LocalCache* cache = nullptr;
  CacheKey key;
  QString rel_path;
  QString final_path;
  QString tmp_path;
  std::shared_ptr<arrow::io::FileOutputStream> sink;
  std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
  bool finished = false;
};

LocalCache::Writer::Writer() = default;

LocalCache::Writer::Writer(std::unique_ptr<Impl> impl)
  : impl_(std::move(impl))
{
}

LocalCache::Writer::~Writer()
{
  abort();
}

LocalCache::Writer::Writer(Writer&& other) noexcept = default;

LocalCache::Writer& LocalCache::Writer::operator=(Writer&& other) noexcept
{
  if (this != &other)
  {
    abort();
    impl_ = std::move(other.impl_);
  }
  return *this;
}

bool LocalCache::Writer::isOpen() const
{
  return impl_ && !impl_->finished && impl_->writer && impl_->sink;
}

bool LocalCache::Writer::writeBatch(const std::shared_ptr<arrow::RecordBatch>& batch,
                                    QString* error)
{
  if (!isOpen())
  {
    if (error)
    {
      *error = QStringLiteral("cache writer is not open");
    }
    return false;
  }
  if (!batch)
  {
    return true;
  }

  const auto status = impl_->writer->WriteRecordBatch(*batch);
  if (!status.ok())
  {
    abort();
    if (error)
    {
      *error = statusToQString(status);
    }
    return false;
  }
  return true;
}

bool LocalCache::Writer::commit(QString* error)
{
  if (!isOpen())
  {
    if (error)
    {
      *error = QStringLiteral("cache writer is not open");
    }
    return false;
  }

  auto status = impl_->writer->Close();
  if (status.ok())
  {
    status = impl_->sink->Close();
  }
  impl_->writer.reset();
  impl_->sink.reset();
  if (!status.ok())
  {
    QFile::remove(impl_->tmp_path);
    impl_->finished = true;
    if (error)
    {
      *error = statusToQString(status);
    }
    return false;
  }

  QByteArray tmp_name = QFile::encodeName(impl_->tmp_path);
  QByteArray final_name = QFile::encodeName(impl_->final_path);
  if (std::rename(tmp_name.constData(), final_name.constData()) != 0)
  {
    const int first_errno = errno;
    QFile::remove(impl_->final_path);
    if (std::rename(tmp_name.constData(), final_name.constData()) != 0)
    {
      QFile::remove(impl_->tmp_path);
      impl_->finished = true;
      if (error)
      {
        *error = QStringLiteral("cannot move cache file into place: %1 (%2)")
                     .arg(impl_->final_path, QString::fromLocal8Bit(std::strerror(first_errno)));
      }
      return false;
    }
  }

  const qint64 size = QFileInfo(impl_->final_path).size();
  const qint64 access = impl_->cache->currentAccessTime();
  QSqlDatabase db = impl_->cache->database();
  if (!db.transaction())
  {
    impl_->finished = true;
    if (error)
    {
      *error = QStringLiteral("cannot begin cache transaction: %1").arg(sqlError(db));
    }
    return false;
  }

  QSqlQuery q(db);
  q.prepare(QStringLiteral("INSERT OR REPLACE INTO chunks "
                           "(dataset_id, dataset_version, topic, chunk_start, chunk_end, path, "
                           "size_bytes, last_access, ticket) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, NULL)"));
  q.addBindValue(impl_->key.dataset_id);
  q.addBindValue(impl_->key.dataset_version);
  q.addBindValue(impl_->key.topic);
  q.addBindValue(impl_->key.chunk_start_ns);
  q.addBindValue(impl_->key.chunk_end_ns);
  q.addBindValue(impl_->rel_path);
  q.addBindValue(size);
  q.addBindValue(access);
  if (!q.exec())
  {
    db.rollback();
    impl_->finished = true;
    if (error)
    {
      *error = QStringLiteral("cannot upsert cache entry: %1").arg(sqlError(q));
    }
    return false;
  }
  if (!db.commit())
  {
    impl_->finished = true;
    if (error)
    {
      *error = QStringLiteral("cannot commit cache entry: %1").arg(sqlError(db));
    }
    return false;
  }

  impl_->finished = true;
  return impl_->cache->evictIfNeeded(error);
}

void LocalCache::Writer::abort()
{
  if (!impl_ || impl_->finished)
  {
    return;
  }

  if (impl_->writer)
  {
    (void)impl_->writer->Close();
    impl_->writer.reset();
  }
  if (impl_->sink)
  {
    (void)impl_->sink->Close();
    impl_->sink.reset();
  }
  if (!impl_->tmp_path.isEmpty())
  {
    QFile::remove(impl_->tmp_path);
  }
  impl_->finished = true;
}

LocalCache::Writer LocalCache::beginWrite(const CacheKey& key,
                                          const std::shared_ptr<arrow::Schema>& schema,
                                          QString* error)
{
  if (!ensureOpen(error))
  {
    return {};
  }
  if (!schema)
  {
    if (error)
    {
      *error = QStringLiteral("cannot cache pull result without a schema");
    }
    return {};
  }

  const QString rel_path = relativePathForKey(key);
  const QString final_path = absolutePath(rel_path);
  QDir dir(QFileInfo(final_path).absolutePath());
  if (!dir.mkpath(QStringLiteral(".")))
  {
    if (error)
    {
      *error = QStringLiteral("cannot create cache data directory: %1").arg(dir.absolutePath());
    }
    return {};
  }

  QTemporaryFile tmp(dir.filePath(QStringLiteral(".tmp_XXXXXX.arrow")));
  tmp.setAutoRemove(false);
  if (!tmp.open())
  {
    if (error)
    {
      *error = QStringLiteral("cannot create temporary cache file: %1").arg(tmp.errorString());
    }
    return {};
  }
  const QString tmp_path = tmp.fileName();
  tmp.close();

  auto sink_result = arrow::io::FileOutputStream::Open(tmp_path.toStdString());
  if (!sink_result.ok())
  {
    QFile::remove(tmp_path);
    if (error)
    {
      *error = statusToQString(sink_result.status());
    }
    return {};
  }
  auto sink = *sink_result;

  auto options_result = makeWriteOptions();
  if (!options_result.ok())
  {
    (void)sink->Close();
    QFile::remove(tmp_path);
    if (error)
    {
      *error = statusToQString(options_result.status());
    }
    return {};
  }

  auto writer_result = arrow::ipc::MakeFileWriter(sink.get(), schema, *options_result);
  if (!writer_result.ok())
  {
    (void)sink->Close();
    QFile::remove(tmp_path);
    if (error)
    {
      *error = statusToQString(writer_result.status());
    }
    return {};
  }

  auto impl = std::make_unique<Writer::Impl>();
  impl->cache = this;
  impl->key = key;
  impl->rel_path = rel_path;
  impl->final_path = final_path;
  impl->tmp_path = tmp_path;
  impl->sink = std::move(sink);
  impl->writer = *writer_result;
  return Writer(std::move(impl));
}

bool LocalCache::write(const CacheKey& key, const PullResult& result, QString* error)
{
  auto schema = schemaFor(result);
  auto pending = beginWrite(key, schema, error);
  if (!pending.isOpen())
  {
    return false;
  }

  for (const auto& batch : result.batches)
  {
    if (!pending.writeBatch(batch, error))
    {
      return false;
    }
  }
  return pending.commit(error);
}

bool LocalCache::updateLastAccess(const CacheKey& key, qint64 access, QString* error)
{
  QSqlQuery q(database());
  q.prepare(QStringLiteral("UPDATE chunks SET last_access=? "
                           "WHERE dataset_id=? AND dataset_version=? AND topic=? "
                           "AND chunk_start=? AND chunk_end=?"));
  q.addBindValue(access);
  q.addBindValue(key.dataset_id);
  q.addBindValue(key.dataset_version);
  q.addBindValue(key.topic);
  q.addBindValue(key.chunk_start_ns);
  q.addBindValue(key.chunk_end_ns);
  if (!q.exec())
  {
    if (error)
    {
      *error = QStringLiteral("cannot update cache access time: %1").arg(sqlError(q));
    }
    return false;
  }
  return true;
}

bool LocalCache::removeEntry(const CacheKey& key, QString* error)
{
  QSqlQuery q(database());
  q.prepare(QStringLiteral("DELETE FROM chunks "
                           "WHERE dataset_id=? AND dataset_version=? AND topic=? "
                           "AND chunk_start=? AND chunk_end=?"));
  q.addBindValue(key.dataset_id);
  q.addBindValue(key.dataset_version);
  q.addBindValue(key.topic);
  q.addBindValue(key.chunk_start_ns);
  q.addBindValue(key.chunk_end_ns);
  if (!q.exec())
  {
    if (error)
    {
      *error = QStringLiteral("cannot delete stale cache entry: %1").arg(sqlError(q));
    }
    return false;
  }
  return true;
}

bool LocalCache::evictIfNeeded(QString* error)
{
  if (max_size_bytes_ <= 0)
  {
    return true;
  }

  QSqlDatabase db = database();
  QSqlQuery total_q(db);
  if (!total_q.exec(QStringLiteral("SELECT COALESCE(SUM(size_bytes), 0) FROM chunks")) ||
      !total_q.next())
  {
    if (error)
    {
      *error = QStringLiteral("cannot compute cache size: %1").arg(sqlError(total_q));
    }
    return false;
  }
  qint64 total = total_q.value(0).toLongLong();
  total_q.finish();
  if (total <= max_size_bytes_)
  {
    return true;
  }

  if (!db.transaction())
  {
    if (error)
    {
      *error = QStringLiteral("cannot begin cache eviction transaction: %1").arg(sqlError(db));
    }
    return false;
  }

  QSqlQuery victims(db);
  if (!victims.exec(QStringLiteral("SELECT dataset_id, dataset_version, topic, chunk_start, "
                                   "chunk_end, path, size_bytes "
                                   "FROM chunks ORDER BY last_access ASC")))
  {
    db.rollback();
    if (error)
    {
      *error = QStringLiteral("cannot select cache eviction victims: %1").arg(sqlError(victims));
    }
    return false;
  }

  struct Victim
  {
    CacheKey key;
    QString path;
    qint64 size = 0;
  };
  QList<Victim> selected;
  while (total > max_size_bytes_ && victims.next())
  {
    Victim v;
    v.key.dataset_id = victims.value(0).toString();
    v.key.dataset_version = victims.value(1).toString();
    v.key.topic = victims.value(2).toString();
    v.key.chunk_start_ns = victims.value(3).toLongLong();
    v.key.chunk_end_ns = victims.value(4).toLongLong();
    v.path = victims.value(5).toString();
    v.size = victims.value(6).toLongLong();
    selected.push_back(v);
    total -= v.size;
  }
  victims.finish();

  QSqlQuery del(db);
  del.prepare(QStringLiteral("DELETE FROM chunks "
                             "WHERE dataset_id=? AND dataset_version=? AND topic=? "
                             "AND chunk_start=? AND chunk_end=?"));
  for (const auto& victim : selected)
  {
    del.bindValue(0, victim.key.dataset_id);
    del.bindValue(1, victim.key.dataset_version);
    del.bindValue(2, victim.key.topic);
    del.bindValue(3, victim.key.chunk_start_ns);
    del.bindValue(4, victim.key.chunk_end_ns);
    if (!del.exec())
    {
      db.rollback();
      if (error)
      {
        *error = QStringLiteral("cannot delete cache eviction victim: %1").arg(sqlError(del));
      }
      return false;
    }
  }

  if (!db.commit())
  {
    if (error)
    {
      *error = QStringLiteral("cannot commit cache eviction: %1").arg(sqlError(db));
    }
    return false;
  }

  for (const auto& victim : selected)
  {
    QFile::remove(absolutePath(victim.path));
  }
  return true;
}

}  // namespace mosaico_cache
