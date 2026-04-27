/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flight/types.hpp>
using mosaico::PullResult;

#include <QSqlDatabase>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>

namespace mosaico_cache
{

struct CacheKey
{
  QString dataset_id;
  QString dataset_version;
  QString topic;
  qint64 chunk_start_ns = 0;
  qint64 chunk_end_ns = 0;
};

struct CacheReadResult
{
  bool hit = false;
  PullResult result;
  QString error;
};

struct CacheStreamResult
{
  bool hit = false;
  qint64 decoded_bytes = 0;
  QString error;
};

class LocalCache
{
public:
  class Writer
  {
  public:
    Writer();
    ~Writer();

    Writer(Writer&& other) noexcept;
    Writer& operator=(Writer&& other) noexcept;

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    [[nodiscard]] bool isOpen() const;
    bool writeBatch(const std::shared_ptr<arrow::RecordBatch>& batch,
                    QString* error = nullptr);
    bool commit(QString* error = nullptr);
    void abort();

  private:
    friend class LocalCache;
    struct Impl;
    explicit Writer(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
  };

  explicit LocalCache(QString root_path = defaultRootPath(), qint64 max_size_bytes = defaultBudget());
  ~LocalCache();

  LocalCache(const LocalCache&) = delete;
  LocalCache& operator=(const LocalCache&) = delete;

  [[nodiscard]] static QString defaultRootPath();
  [[nodiscard]] static qint64 defaultBudget();

  [[nodiscard]] const QString& rootPath() const
  {
    return root_path_;
  }

  [[nodiscard]] bool open(QString* error = nullptr);
  [[nodiscard]] CacheReadResult read(const CacheKey& key);
  [[nodiscard]] CacheStreamResult
  readStreaming(const CacheKey& key,
                const std::function<void(const std::shared_ptr<arrow::Schema>&)>& schema_cb,
                const std::function<bool(const std::shared_ptr<arrow::RecordBatch>&)>& batch_cb);
  [[nodiscard]] Writer beginWrite(const CacheKey& key,
                                  const std::shared_ptr<arrow::Schema>& schema,
                                  QString* error = nullptr);
  bool write(const CacheKey& key, const PullResult& result, QString* error = nullptr);

private:
  [[nodiscard]] bool ensureOpen(QString* error);
  [[nodiscard]] QSqlDatabase database() const;
  [[nodiscard]] bool execPragma(const QString& sql, QString* error);
  [[nodiscard]] bool ensureSchema(QString* error);
  [[nodiscard]] QString relativePathForKey(const CacheKey& key) const;
  [[nodiscard]] QString absolutePath(const QString& relative_path) const;
  [[nodiscard]] qint64 currentAccessTime() const;
  bool updateLastAccess(const CacheKey& key, qint64 access, QString* error);
  bool removeEntry(const CacheKey& key, QString* error);
  bool evictIfNeeded(QString* error);

  QString root_path_;
  qint64 max_size_bytes_ = 0;
  QString connection_name_;
  bool opened_ = false;
};

}  // namespace mosaico_cache
