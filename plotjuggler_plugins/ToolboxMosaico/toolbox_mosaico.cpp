/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "toolbox_mosaico.h"
#include "src/ui/main_window.h"
#include "src/ui/theme_utils.h"

#include <QDebug>
#include <QMetaType>
#include <QSettings>
#include <arrow/api.h>
#include <arrow/array/array_nested.h>
#include <arrow/type.h>

#include "src/query/query.h"

#include <cmath>
#include <exception>
#include <limits>
#include <set>
#include <utility>

namespace
{

// Plugin-boundary guard. Any exception that reaches a ToolboxPlugin
// override or queued-connection slot must not propagate into PlotJuggler,
// which would terminate the host application. The contract the user
// agreed to: worst case = plugin closes, not app crashes. `on_fail` is
// the action to take after logging — typically `emit closed()` at the
// boundary overrides, or a no-op for pure slots.
template <typename Fn, typename OnFail>
void guardedBoundary(const char* where, Fn&& fn, OnFail&& on_fail)
{
  try
  {
    std::forward<Fn>(fn)();
    return;
  }
  catch (const std::exception& e)
  {
    qWarning() << "[Mosaico plugin]" << where << "exception:" << e.what();
  }
  catch (...)
  {
    qWarning() << "[Mosaico plugin]" << where << "exception: unknown";
  }
  std::forward<OnFail>(on_fail)();
}

// Matches the pattern used by DataLoadParquet for Arrow type -> double conversion.
template <typename ArrowArrayType>
double getArrowValue(const std::shared_ptr<arrow::Array>& array, int64_t index)
{
  auto typed = std::static_pointer_cast<ArrowArrayType>(array);
  if (typed->IsNull(index))
  {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return static_cast<double>(typed->Value(index));
}

double getArrowValue(const std::shared_ptr<arrow::Array>& array, int64_t index,
                     arrow::Type::type type)
{
  switch (type)
  {
    case arrow::Type::BOOL:
      return getArrowValue<arrow::BooleanArray>(array, index);
    case arrow::Type::INT8:
      return getArrowValue<arrow::Int8Array>(array, index);
    case arrow::Type::INT16:
      return getArrowValue<arrow::Int16Array>(array, index);
    case arrow::Type::INT32:
      return getArrowValue<arrow::Int32Array>(array, index);
    case arrow::Type::INT64:
      return getArrowValue<arrow::Int64Array>(array, index);
    case arrow::Type::UINT8:
      return getArrowValue<arrow::UInt8Array>(array, index);
    case arrow::Type::UINT16:
      return getArrowValue<arrow::UInt16Array>(array, index);
    case arrow::Type::UINT32:
      return getArrowValue<arrow::UInt32Array>(array, index);
    case arrow::Type::UINT64:
      return getArrowValue<arrow::UInt64Array>(array, index);
    case arrow::Type::FLOAT:
      return getArrowValue<arrow::FloatArray>(array, index);
    case arrow::Type::DOUBLE:
      return getArrowValue<arrow::DoubleArray>(array, index);
    case arrow::Type::TIMESTAMP: {
      auto ts_arr = std::static_pointer_cast<arrow::TimestampArray>(array);
      if (ts_arr->IsNull(index))
      {
        return std::numeric_limits<double>::quiet_NaN();
      }
      auto ts_type = std::static_pointer_cast<arrow::TimestampType>(ts_arr->type());
      double seconds = static_cast<double>(ts_arr->Value(index));
      switch (ts_type->unit())
      {
        case arrow::TimeUnit::SECOND:
          break;
        case arrow::TimeUnit::MILLI:
          seconds /= 1e3;
          break;
        case arrow::TimeUnit::MICRO:
          seconds /= 1e6;
          break;
        case arrow::TimeUnit::NANO:
          seconds /= 1e9;
          break;
      }
      return seconds;
    }
    default:
      return std::numeric_limits<double>::quiet_NaN();
  }
}

bool isNumericType(arrow::Type::type type)
{
  switch (type)
  {
    case arrow::Type::BOOL:
    case arrow::Type::INT8:
    case arrow::Type::INT16:
    case arrow::Type::INT32:
    case arrow::Type::INT64:
    case arrow::Type::UINT8:
    case arrow::Type::UINT16:
    case arrow::Type::UINT32:
    case arrow::Type::UINT64:
    case arrow::Type::FLOAT:
    case arrow::Type::DOUBLE:
      return true;
    default:
      return false;
  }
}

// Reads the timestamp value and always returns seconds.
// For TIMESTAMP columns, getArrowValue already converts to seconds.
// For INT64 columns found by name (e.g. "recording_timestamp_ns"), the raw
// value is nanoseconds — divide by 1e9.
double getTimestampSeconds(const std::shared_ptr<arrow::Array>& ts_array, int64_t row,
                           arrow::Type::type ts_type, bool ts_is_nanoseconds)
{
  double raw = getArrowValue(ts_array, row, ts_type);
  if (ts_is_nanoseconds && ts_type != arrow::Type::TIMESTAMP)
  {
    raw /= 1e9;
  }
  return raw;
}

// Returns true if a field name looks like a timestamp column that shouldn't
// be emitted as a data series (e.g. nested "recording_timestamp_ns" inside structs).
bool isTimestampFieldName(const std::string& name)
{
  return name == "timestamp_ns" || name == "recording_timestamp_ns" || name == "timestamp" ||
         name == "time" || name == "ts";
}

std::string topicImportKey(const QString& sequence_name, const QString& topic_name)
{
  return sequence_name.toStdString() + '\n' + topic_name.toStdString();
}

}  // anonymous namespace

ToolboxMosaico::ToolboxMosaico()
{
  // Register custom types so queued signal/slot connections can marshal
  // them across the FetchWorker thread boundary.
  qRegisterMetaType<PullResult>("PullResult");
  qRegisterMetaType<std::shared_ptr<arrow::Schema>>("std::shared_ptr<arrow::Schema>");
  qRegisterMetaType<std::shared_ptr<arrow::RecordBatch>>("std::shared_ptr<arrow::RecordBatch>");
  qRegisterMetaType<SequenceInfo>("SequenceInfo");
  qRegisterMetaType<TopicInfo>("TopicInfo");
  qRegisterMetaType<std::vector<SequenceInfo>>("std::vector<SequenceInfo>");
  qRegisterMetaType<std::vector<TopicInfo>>("std::vector<TopicInfo>");
}

ToolboxMosaico::~ToolboxMosaico() = default;

void ToolboxMosaico::init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& /*transform_map*/)
{
  guardedBoundary(
      "init",
      [&]() {
        plot_data_ = &src_data;
        main_window_ = new MainWindow(nullptr);

        connect(main_window_, &MainWindow::mosaicoDataReady, this,
                &ToolboxMosaico::onMosaicoDataReady);
        connect(main_window_, &MainWindow::mosaicoTopicStarted, this,
                &ToolboxMosaico::onMosaicoTopicStarted);
        connect(main_window_, &MainWindow::mosaicoTopicBatchReady, this,
                &ToolboxMosaico::onMosaicoTopicBatchReady);
        connect(main_window_, &MainWindow::mosaicoTopicFinished, this,
                &ToolboxMosaico::onMosaicoTopicFinished);
        connect(main_window_, &MainWindow::allFetchesComplete, this,
                &ToolboxMosaico::onAllFetchesComplete);
        // Drop any partial data that made it in before the user cancelled —
        // otherwise it would leak into the next fetch cycle's import.
        connect(main_window_, &MainWindow::fetchCancelled, this, [this]() {
          imported_data_ = PJ::PlotDataMapRef();
          topic_imports_.clear();
        });
        connect(main_window_, &MainWindow::backRequested, this, [this]() { emit closed(); });
        connect(main_window_, &MainWindow::schemaReady, this, &ToolboxMosaico::onSchemaReady);
        connect(main_window_, &MainWindow::queryChanged, this, &ToolboxMosaico::onQueryChanged);
      },
      [this]() { emit closed(); });
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxMosaico::providedWidget() const
{
  // providedWidget() is const, can't emit closed() from here. The pair
  // constructor is noexcept for pointer types, so there is no realistic
  // throw — but keep the call site minimal so nothing new is introduced.
  return { main_window_, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxMosaico::onShowWidget()
{
  bool ok = true;
  guardedBoundary(
      "onShowWidget",
      [&]() {
        main_window_->updateTheme(isDarkTheme());

        if (!initialized_)
        {
          QSettings settings;
          main_window_->restoreState(settings);

          // Auto-connect with saved URI — errors go to status label only (no popup).
          main_window_->connectToServer(/*explicit_connect=*/false);
          initialized_ = true;
        }
      },
      [this, &ok]() {
        ok = false;
        emit closed();
      });
  return ok;
}

void ToolboxMosaico::onMosaicoDataReady(const QString& sequence_name, const QString& topic_name,
                                        const PullResult& result)
{
  // Arrow pointer casts in convertRecordBatchToSeries can throw std::bad_cast
  // on unexpected schemas from the server; catch and close the plugin instead
  // of letting the exception reach PlotJuggler's event loop.
  guardedBoundary(
      "onMosaicoDataReady",
      [&]() { convertRecordBatchToSeries(sequence_name, topic_name, result); },
      [this]() { emit closed(); });
}

void ToolboxMosaico::onMosaicoTopicStarted(const QString& sequence_name, const QString& topic_name,
                                           const std::shared_ptr<arrow::Schema>& schema)
{
  guardedBoundary(
      "onMosaicoTopicStarted", [&]() { beginTopicImport(sequence_name, topic_name, schema); },
      [this]() { emit closed(); });
}

void ToolboxMosaico::onMosaicoTopicBatchReady(const QString& sequence_name,
                                              const QString& topic_name,
                                              const std::shared_ptr<arrow::RecordBatch>& batch)
{
  guardedBoundary(
      "onMosaicoTopicBatchReady",
      [&]() { appendRecordBatchToSeries(sequence_name, topic_name, batch); },
      [this]() { emit closed(); });
}

void ToolboxMosaico::onMosaicoTopicFinished(const QString& sequence_name, const QString& topic_name)
{
  guardedBoundary(
      "onMosaicoTopicFinished",
      [&]() { topic_imports_.erase(topicImportKey(sequence_name, topic_name)); },
      [this]() { emit closed(); });
}

void ToolboxMosaico::onAllFetchesComplete()
{
  guardedBoundary(
      "onAllFetchesComplete",
      [&]() {
        // Save UI state before hiding
        QSettings settings;
        main_window_->saveState(settings);

        // Only import and close if we actually received data.
        // When all fetches failed, imported_data_ is empty — keep the plugin
        // open so the user can see the error in the status label and retry.
        if (!imported_data_.numeric.empty() || !imported_data_.strings.empty())
        {
          emit importData(imported_data_, false);
          imported_data_ = PJ::PlotDataMapRef();  // Reset for next fetch cycle
          emit closed();
        }
      },
      [this]() { emit closed(); });
}

// Recursively flattens an Arrow array into PlotJuggler series.
// For numeric arrays, creates a series directly.
// For struct arrays, recurses into each child field, building the path with '/'.
void ToolboxMosaico::flattenArray(const std::shared_ptr<arrow::Array>& array,
                                  const std::shared_ptr<arrow::Array>& ts_array,
                                  arrow::Type::type ts_type, bool ts_is_ns, int64_t num_rows,
                                  const std::string& path, std::set<std::string>& created_series)
{
  auto type_id = array->type_id();

  if (isNumericType(type_id) || type_id == arrow::Type::TIMESTAMP)
  {
    auto& series = imported_data_.getOrCreateNumeric(path);
    created_series.insert(path);
    for (int64_t row = 0; row < num_rows; ++row)
    {
      double t = getTimestampSeconds(ts_array, row, ts_type, ts_is_ns);
      double v = getArrowValue(array, row, type_id);
      series.pushBack({ t, v });
    }
    return;
  }

  if (type_id == arrow::Type::STRUCT)
  {
    auto struct_arr = std::static_pointer_cast<arrow::StructArray>(array);
    auto struct_type = std::static_pointer_cast<arrow::StructType>(array->type());
    for (int i = 0; i < struct_type->num_fields(); ++i)
    {
      const auto& child_name = struct_type->field(i)->name();
      // Skip timestamp fields nested inside structs — they're not data.
      if (isTimestampFieldName(child_name))
      {
        continue;
      }
      auto child_path = path + "/" + child_name;
      flattenArray(struct_arr->field(i), ts_array, ts_type, ts_is_ns, num_rows, child_path,
                   created_series);
    }
  }
}

void ToolboxMosaico::convertRecordBatchToSeries(const QString& sequence_name,
                                                const QString& topic_name, const PullResult& result)
{
  if (!beginTopicImport(sequence_name, topic_name, result.schema))
  {
    return;
  }
  for (const auto& batch : result.batches)
  {
    appendRecordBatchToSeries(sequence_name, topic_name, batch);
  }
  topic_imports_.erase(topicImportKey(sequence_name, topic_name));
}

bool ToolboxMosaico::beginTopicImport(const QString& sequence_name, const QString& topic_name,
                                      const std::shared_ptr<arrow::Schema>& schema)
{
  if (!schema)
  {
    return false;
  }
  // Find timestamp column by Arrow type
  int ts_col = -1;
  for (int i = 0; i < schema->num_fields(); ++i)
  {
    if (schema->field(i)->type()->id() == arrow::Type::TIMESTAMP)
    {
      ts_col = i;
      break;
    }
  }
  // Fallback: find by name
  if (ts_col < 0)
  {
    for (const auto& name : { "timestamp_ns", "recording_timestamp_ns", "timestamp", "time", "ts" })
    {
      ts_col = schema->GetFieldIndex(name);
      if (ts_col >= 0)
      {
        break;
      }
    }
  }
  if (ts_col < 0)
  {
    return false;
  }

  auto ts_type = schema->field(ts_col)->type()->id();
  bool ts_is_ns = (ts_type != arrow::Type::TIMESTAMP) &&
                  (schema->field(ts_col)->name().find("_ns") != std::string::npos ||
                   schema->field(ts_col)->name() == "timestamp");

  const std::string prefix = sequence_name.toStdString() + "/" + topic_name.toStdString();

  // Clear data points in existing series with the same prefix so re-fetching
  // replaces old data. We clear() rather than erase() because erasing invalidates
  // references held by the tree view and active plots, causing a crash.
  if (plot_data_)
  {
    for (auto& [name, series] : plot_data_->numeric)
    {
      if (name.compare(0, prefix.size(), prefix) == 0)
      {
        series.clear();
      }
    }
  }

  TopicImportState state;
  state.schema = schema;
  state.ts_col = ts_col;
  state.ts_type = ts_type;
  state.ts_is_ns = ts_is_ns;
  state.prefix = prefix;
  topic_imports_[topicImportKey(sequence_name, topic_name)] = std::move(state);
  return true;
}

void ToolboxMosaico::appendRecordBatchToSeries(const QString& sequence_name,
                                               const QString& topic_name,
                                               const std::shared_ptr<arrow::RecordBatch>& batch)
{
  if (!batch)
  {
    return;
  }
  auto it = topic_imports_.find(topicImportKey(sequence_name, topic_name));
  if (it == topic_imports_.end())
  {
    if (!beginTopicImport(sequence_name, topic_name, batch->schema()))
    {
      return;
    }
    it = topic_imports_.find(topicImportKey(sequence_name, topic_name));
    if (it == topic_imports_.end())
    {
      return;
    }
  }

  auto& state = it->second;
  if (state.ts_col < 0 || state.ts_col >= batch->num_columns())
  {
    return;
  }

  auto ts_array = batch->column(state.ts_col);

  for (int col = 0; col < batch->num_columns(); ++col)
  {
    if (col == state.ts_col)
    {
      continue;
    }
    auto field = batch->schema()->field(col);
    if (isTimestampFieldName(field->name()))
    {
      continue;
    }
    auto col_path = state.prefix + "/" + field->name();
    flattenArray(batch->column(col), ts_array, state.ts_type, state.ts_is_ns, batch->num_rows(),
                 col_path, state.created_series);
  }
}

void ToolboxMosaico::onSchemaReady(const Schema& schema)
{
  guardedBoundary(
      "onSchemaReady", [&]() { schema_ = schema; },
      []() { /* non-critical — schema stays stale until next fetch */ });
}

void ToolboxMosaico::onQueryChanged(const QString& query, bool valid)
{
  // Query::Query(...) and Engine::eval can throw on malformed queries
  // (e.g. if the validator and parser disagree); swallowing here means a
  // bad query blanks the visible set rather than taking down PlotJuggler.
  guardedBoundary(
      "onQueryChanged",
      [&]() {
        if (query.isEmpty())
        {
          main_window_->clearVisibleSequences();
          return;
        }
        if (!valid)
        {
          return;
        }

        auto query_str = query.toStdString();
        Query q(query_str, schema_);

        std::set<std::string> visible;
        Metadata prev_md;
        for (const auto& seq : main_window_->sequences())
        {
          engine_.clear(prev_md);
          Metadata md(seq.user_metadata.begin(), seq.user_metadata.end());
          engine_.set(md);
          prev_md = md;
          if (engine_.eval(q))
          {
            visible.insert(seq.name);
          }
        }
        engine_.clear(prev_md);
        main_window_->setVisibleSequences(visible);
      },
      [this]() { main_window_->clearVisibleSequences(); });
}
