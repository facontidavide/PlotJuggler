/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "cache/local_cache.h"

#include <arrow/api.h>
#include <gtest/gtest.h>

#include <QSqlDatabase>
#include <QTemporaryDir>

#include <memory>

namespace
{

bool hasSqliteDriver()
{
  return QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"));
}

mosaico::PullResult makePullResult()
{
  arrow::Int64Builder ts_builder;
  EXPECT_TRUE(ts_builder.AppendValues({ 10, 20 }).ok());
  std::shared_ptr<arrow::Array> ts_array;
  EXPECT_TRUE(ts_builder.Finish(&ts_array).ok());

  arrow::DoubleBuilder value_builder;
  EXPECT_TRUE(value_builder.AppendValues({ 1.5, 2.5 }).ok());
  std::shared_ptr<arrow::Array> value_array;
  EXPECT_TRUE(value_builder.Finish(&value_array).ok());

  auto schema = arrow::schema({ arrow::field("timestamp_ns", arrow::int64()),
                                arrow::field("value", arrow::float64()) });

  mosaico::PullResult result;
  result.schema = schema;
  result.batches.push_back(arrow::RecordBatch::Make(schema, 2, { ts_array, value_array }));
  return result;
}

mosaico_cache::CacheKey makeKey()
{
  mosaico_cache::CacheKey key;
  key.dataset_id = QStringLiteral("demo.mosaico.dev:6726/sequence-a");
  key.dataset_version = QStringLiteral("unversioned");
  key.topic = QStringLiteral("/robot/state");
  key.chunk_start_ns = 100;
  key.chunk_end_ns = 200;
  return key;
}

}  // namespace

TEST(LocalCache, MissingEntryIsMiss)
{
  if (!hasSqliteDriver())
  {
    GTEST_SKIP() << "Qt QSQLITE driver is not available";
  }

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  mosaico_cache::LocalCache cache(dir.path());
  auto result = cache.read(makeKey());
  EXPECT_FALSE(result.hit);
}

TEST(LocalCache, WritesAndReadsArrowIpc)
{
  if (!hasSqliteDriver())
  {
    GTEST_SKIP() << "Qt QSQLITE driver is not available";
  }

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  mosaico_cache::LocalCache cache(dir.path());
  QString error;
  ASSERT_TRUE(cache.open(&error)) << error.toStdString();

  auto input = makePullResult();
  ASSERT_TRUE(cache.write(makeKey(), input, &error)) << error.toStdString();

  auto cached = cache.read(makeKey());
  ASSERT_TRUE(cached.hit) << cached.error.toStdString();
  ASSERT_TRUE(cached.result.schema);
  ASSERT_EQ(cached.result.schema->num_fields(), 2);
  ASSERT_EQ(cached.result.batches.size(), 1UL);
  ASSERT_EQ(cached.result.batches[0]->num_rows(), 2);

  auto values =
      std::static_pointer_cast<arrow::DoubleArray>(cached.result.batches[0]->column(1));
  EXPECT_DOUBLE_EQ(values->Value(0), 1.5);
  EXPECT_DOUBLE_EQ(values->Value(1), 2.5);
}

TEST(LocalCache, EvictsWhenBudgetIsExceeded)
{
  if (!hasSqliteDriver())
  {
    GTEST_SKIP() << "Qt QSQLITE driver is not available";
  }

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());

  mosaico_cache::LocalCache cache(dir.path(), /*max_size_bytes=*/1);
  QString error;
  ASSERT_TRUE(cache.write(makeKey(), makePullResult(), &error)) << error.toStdString();

  auto cached = cache.read(makeKey());
  EXPECT_FALSE(cached.hit);
}
