#include "toolbox_csv.h"

#if TOOLBOXCSV_WITH_PARQUET
#ifdef signals
#undef signals
#endif

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>
#endif

#include <algorithm>
#include <limits>
#include <map>

#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QDir>
#include <QSet>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>
#include <QEvent>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDataStream>
#include <QToolButton>

ToolboxCSV::ToolboxCSV()
{
  // Remove selected rows bottom-up to keep indices valid.
  connect(&_ui, &ToolBoxUI::removeRequested, this, [this]() {
    _ui.clearTable(false);
    _ui.updateTimeControlsEnabled();
    updateTimeRange();
  });

  // Clear all topics.
  connect(&_ui, &ToolBoxUI::clearRequested, this, [this]() {
    _ui.clearTable(true);
    _ui.updateTimeControlsEnabled();
    updateTimeRange();
  });

  // Add all topics of the data (numeric + string series).
  connect(&_ui, &ToolBoxUI::addAllRequested, this, [this]() {
    if (!_plot_data)
    {
      return;
    }

    std::vector<std::string> topics;
    topics.reserve(_plot_data->numeric.size() + _plot_data->strings.size());

    for (const auto& [name, _] : _plot_data->numeric)
    {
      topics.push_back(name);
    }
    for (const auto& [name, _] : _plot_data->strings)
    {
      topics.push_back(name);
    }

    std::sort(topics.begin(), topics.end());
    _ui.setTopics(topics);
  });

  // Recompute visible time range when switching relative/absolute or table changed.
  connect(&_ui, &ToolBoxUI::recomputeTime, this, &ToolboxCSV::updateTimeRange);

  // Close/cancel
  connect(&_ui, &ToolBoxUI::closed, this, &ToolboxCSV::onClosed);

  // Exporters.
  connect(&_ui, &ToolBoxUI::exportSingleFile, this, &ToolboxCSV::onExportSingleFile);
  connect(&_ui, &ToolBoxUI::exportMultipleFiles, this, &ToolboxCSV::onExportMultipleFiles);
}

ToolboxCSV::~ToolboxCSV()
{
}

void ToolboxCSV::init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxCSV::providedWidget() const
{
  return { _ui.widget(), PJ::ToolboxPlugin::WidgetType::FIXED };
}

bool ToolboxCSV::onShowWidget()
{
  _ui.updateTimeControlsEnabled();

  // Recompute time range on show in case selected topics changed.
  updateTimeRange();

  return true;
}

bool ToolboxCSV::serializeTable(const ExportTable& table, const QString& path, bool is_csv)
{
  if (path.isEmpty() || table.time.empty() || table.cols.size() != table.names.size() ||
      table.string_cols.size() != table.string_names.size())
  {
    return false;
  }
  if (is_csv)
  {
    return serializeCSV(table, path);
  }
#if TOOLBOXCSV_WITH_PARQUET
  return serializeParquet(table, path);
#else
  QMessageBox::warning(_ui.widget(), "Arrow/Parquet", "Can't load Arrow/Parquet");
  return false;
#endif
}

void ToolboxCSV::onExportSingleFile(bool is_csv, QString filename)
{
  const auto [t_start, t_end] = _ui.getAbsoluteTimeRange();

  std::vector<std::string> selected_topics = _ui.getSelectedTopics();
  if (selected_topics.empty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "No topics selected.");
    return;
  }

  ExportTable table = buildExportTable(selected_topics, t_start, t_end);
  if (table.time.empty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "No samples found in the selected time range.");
    return;
  }

  if (!serializeTable(table, filename, is_csv))
  {
    QMessageBox::warning(_ui.widget(), "Export", "Failed to write the output file.");
    return;
  }

  const QString folder = QFileInfo(filename).absolutePath();
  const QString fname = QFileInfo(filename).fileName();
  QMessageBox::information(
      _ui.widget(), "Export",
      QString("<b>File saved in folder:</b><br>%1<br><br><b>File name:</b><br>%2")
          .arg(folder.toHtmlEscaped(), fname.toHtmlEscaped()));
  emit closed();
}

void ToolboxCSV::onExportMultipleFiles(bool is_csv, QDir dir, QString prefix)
{
  const auto [t_start, t_end] = _ui.getAbsoluteTimeRange();

  std::vector<std::string> selected_topics = _ui.getSelectedTopics();
  if (selected_topics.empty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "No topics selected.");
    return;
  }

  const QString ext = is_csv ? "csv" : "parquet";

  // Group selected topics by their PlotGroup name (check numeric, then string).
  std::map<std::string, std::vector<std::string>> groups;

  for (const auto& topic_name : selected_topics)
  {
    std::string group_name = "ungrouped";

    auto it_num = _plot_data->numeric.find(topic_name);
    if (it_num != _plot_data->numeric.end())
    {
      const auto& gp = it_num->second.group();
      if (gp)
      {
        group_name = gp->name();
      }
    }
    else
    {
      auto it_str = _plot_data->strings.find(topic_name);
      if (it_str == _plot_data->strings.end())
      {
        continue;  // topic not found in either map
      }
      const auto& gp = it_str->second.group();
      if (gp)
      {
        group_name = gp->name();
      }
    }

    groups[group_name].push_back(topic_name);
  }

  QStringList saved_files;

  for (const auto& [group_name, topics] : groups)
  {
    ExportTable table = buildExportTable(topics, t_start, t_end);
    if (table.time.empty())
    {
      continue;
    }

    // Strip group prefix from column names (e.g. "/robot/imu/accel" -> "accel").
    auto strip_prefix = [&group_name](std::vector<std::string>& names) {
      for (auto& col_name : names)
      {
        if (col_name.size() > group_name.size() &&
            col_name.compare(0, group_name.size(), group_name) == 0)
        {
          size_t start = group_name.size();
          if (start < col_name.size() && col_name[start] == '/')
          {
            start++;
          }
          col_name.erase(0, start);
        }
      }
    };
    strip_prefix(table.names);
    strip_prefix(table.string_names);

    // Sanitize group name for use in filename.
    QString safe_group = QString::fromStdString(group_name);
    safe_group.replace(QChar('/'), QChar('_'));
    safe_group.replace(QChar('\\'), QChar('_'));
    safe_group.replace(QChar(':'), QChar('_'));

    const QString filename = dir.filePath(QString("%1_%2.%3").arg(prefix, safe_group, ext));

    if (!serializeTable(table, filename, is_csv))
    {
      QMessageBox::warning(_ui.widget(), "Export",
                           QString("Failed to write file: %1").arg(filename));
      return;
    }

    saved_files.append(filename);
  }

  if (saved_files.isEmpty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "No samples found in the selected time range.");
    return;
  }

  QStringList file_names;
  for (const QString& f : saved_files)
  {
    file_names.append(QFileInfo(f).fileName().toHtmlEscaped());
  }
  const QString names_label = (saved_files.size() == 1) ? "File name:" : "File names:";
  QMessageBox::information(
      _ui.widget(), "Export",
      QString("<b>Files saved in folder:</b><br>%1<br><br><b>%2</b><br>%3")
          .arg(dir.absolutePath().toHtmlEscaped(), names_label, file_names.join("<br>")));
  emit closed();
}

void ToolboxCSV::onClosed()
{
  emit closed();
}

// Compute global [tmin, tmax] across selected topics (numeric + string).
bool ToolboxCSV::getTimeRange(double& tmin, double& tmax) const
{
  bool any = false;

  const auto topics = _ui.getSelectedTopics();

  // Helper to update bounds from a single series.
  auto update_bounds = [&](double front_time, double back_time) {
    if (!any)
    {
      tmin = front_time;
      tmax = back_time;
      any = true;
    }
    else
    {
      tmin = std::min(tmin, front_time);
      tmax = std::max(tmax, back_time);
    }
  };

  for (const auto& name : topics)
  {
    // Check numeric series.
    auto it_num = _plot_data->numeric.find(name);
    if (it_num != _plot_data->numeric.end() && it_num->second.size() > 0)
    {
      update_bounds(it_num->second.front().x, it_num->second.back().x);
      continue;
    }

    // Check string series.
    auto it_str = _plot_data->strings.find(name);
    if (it_str != _plot_data->strings.end() && it_str->second.size() > 0)
    {
      update_bounds(it_str->second.front().x, it_str->second.back().x);
    }
  }

  if (!any)
  {
    return false;
  }
  if (tmax < tmin)
  {
    std::swap(tmin, tmax);
  }
  return true;
}

void ToolboxCSV::updateTimeRange()
{
  double tmin, tmax;
  if (!getTimeRange(tmin, tmax))
  {
    return;  // No valid topics -> keep current UI range
  }
  _ui.setTimeRange(tmin, tmax);
}

// Estimate local minimum dt to derive adaptive merge tolerance.
template <typename TSeries>
double ToolboxCSV::estimateMinDt(const TSeries& plot, size_t start_idx, double t_end)
{
  if (plot.size() < 2 || start_idx + 1 >= plot.size())
  {
    return 0.0;
  }

  double min_dt = std::numeric_limits<double>::max();
  size_t last = std::min(plot.size() - 1, start_idx + 2000);

  for (size_t idx = start_idx + 1; idx <= last; idx++)
  {
    const double t0 = plot.at(idx - 1).x;
    const double t1 = plot.at(idx).x;
    if (t0 > t_end)
    {
      break;
    }
    const double dt = t1 - t0;
    if (dt > 0.0 && dt < min_dt)
    {
      min_dt = dt;
    }
  }

  if (min_dt == std::numeric_limits<double>::max())
  {
    return 0.0;
  }
  return min_dt;
}

template double ToolboxCSV::estimateMinDt<PJ::PlotData>(const PJ::PlotData&, size_t, double);
template double ToolboxCSV::estimateMinDt<PJ::StringSeries>(const PJ::StringSeries&, size_t,
                                                            double);

// Merge multiple time series into a row-aligned table using adaptive tolerance.
ToolboxCSV::ExportTable ToolboxCSV::buildExportTable(const std::vector<std::string>& topics,
                                                     double t_start, double t_end) const
{
  using ExportTable = ToolboxCSV::ExportTable;
  ExportTable table;

  struct SeriesRef
  {
    std::string name;
    const PJ::PlotData* plot;
    size_t idx;
  };

  struct StringSeriesRef
  {
    std::string name;
    const PJ::StringSeries* plot;
    size_t idx;
  };

  // Collect numeric series.
  std::vector<SeriesRef> series;
  series.reserve(topics.size());

  for (const auto& name : topics)
  {
    auto it = _plot_data->numeric.find(name);
    if (it == _plot_data->numeric.end())
    {
      continue;
    }

    const auto& plot = it->second;
    if (plot.size() == 0 || plot.front().x > t_end || plot.back().x < t_start)
    {
      continue;
    }

    int index = plot.getIndexFromX(t_start);
    if (index < 0)
    {
      continue;
    }

    series.push_back({ name, &plot, static_cast<size_t>(index) });
  }

  // Collect string series.
  std::vector<StringSeriesRef> str_series;

  for (const auto& name : topics)
  {
    auto it = _plot_data->strings.find(name);
    if (it == _plot_data->strings.end())
    {
      continue;
    }

    const auto& plot = it->second;
    if (plot.size() == 0 || plot.front().x > t_end || plot.back().x < t_start)
    {
      continue;
    }

    int index = plot.getIndexFromX(t_start);
    if (index < 0)
    {
      continue;
    }

    str_series.push_back({ name, &plot, static_cast<size_t>(index) });
  }

  if (series.empty() && str_series.empty())
  {
    return table;
  }

  // Compute tolerance from the minimum observed sample period across both types.
  double min_dt = 0.0;
  {
    double best = std::numeric_limits<double>::max();
    for (const auto& sr : series)
    {
      double dt = estimateMinDt(*sr.plot, sr.idx, t_end);
      if (dt > 0.0 && dt < best)
      {
        best = dt;
      }
    }
    for (const auto& sr : str_series)
    {
      double dt = estimateMinDt(*sr.plot, sr.idx, t_end);
      if (dt > 0.0 && dt < best)
      {
        best = dt;
      }
    }
    if (best != std::numeric_limits<double>::max())
    {
      min_dt = best;
    }
  }

  // Tolerance = 0.5 * min_dt to avoid merging consecutive distinct samples.
  const double tol = (min_dt > 0.0) ? (0.5 * min_dt) : 0.0;

  const size_t N = series.size();
  const size_t S = str_series.size();

  // Initialize numeric columns.
  table.names.reserve(N);
  table.cols.assign(N, {});
  table.has_value.assign(N, {});
  for (const auto& sr : series)
  {
    table.names.push_back(sr.name);
  }

  // Initialize string columns.
  table.string_names.reserve(S);
  table.string_cols.assign(S, {});
  table.string_has_value.assign(S, {});
  for (const auto& sr : str_series)
  {
    table.string_names.push_back(sr.name);
  }

  const auto NaN = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> row_values(N, NaN);
  std::vector<bool> row_used(N, false);

  std::vector<std::string> str_row_values(S);
  std::vector<bool> str_row_used(S, false);

  while (true)
  {
    // Find next row time as the minimum current timestamp among all series.
    bool done = true;
    double min_time = std::numeric_limits<double>::max();

    for (size_t col = 0; col < N; col++)
    {
      auto& sr = series[col];
      if (sr.idx >= sr.plot->size())
      {
        continue;
      }

      const auto& point = sr.plot->at(sr.idx);
      if (point.x > t_end)
      {
        continue;
      }

      done = false;
      if (point.x < min_time)
      {
        min_time = point.x;
      }
    }

    for (size_t col = 0; col < S; col++)
    {
      auto& sr = str_series[col];
      if (sr.idx >= sr.plot->size())
      {
        continue;
      }

      const auto& point = sr.plot->at(sr.idx);
      if (point.x > t_end)
      {
        continue;
      }

      done = false;
      if (point.x < min_time)
      {
        min_time = point.x;
      }
    }

    if (done || min_time > t_end)
    {
      break;
    }

    // Reset row buffers.
    std::fill(row_values.begin(), row_values.end(), NaN);
    std::fill(row_used.begin(), row_used.end(), false);
    std::fill(str_row_used.begin(), str_row_used.end(), false);

    // Fill numeric values within tolerance.
    for (size_t col = 0; col < N; col++)
    {
      auto& sr = series[col];
      if (sr.idx >= sr.plot->size())
      {
        continue;
      }

      const auto& point = sr.plot->at(sr.idx);
      if (point.x > t_end)
      {
        continue;
      }

      const bool match =
          (tol == 0.0) ? (point.x == min_time) : (std::abs(point.x - min_time) <= tol);
      if (match)
      {
        row_values[col] = point.y;
        row_used[col] = true;
      }
    }

    // Fill string values within tolerance.
    for (size_t col = 0; col < S; col++)
    {
      auto& sr = str_series[col];
      if (sr.idx >= sr.plot->size())
      {
        continue;
      }

      const auto& point = sr.plot->at(sr.idx);
      if (point.x > t_end)
      {
        continue;
      }

      const bool match =
          (tol == 0.0) ? (point.x == min_time) : (std::abs(point.x - min_time) <= tol);
      if (match)
      {
        str_row_values[col] = std::string(sr.plot->getString(point.y));
        str_row_used[col] = true;
      }
    }

    // Append row to table.
    table.time.push_back(min_time);

    for (size_t col = 0; col < N; col++)
    {
      table.cols[col].push_back(row_values[col]);
      table.has_value[col].push_back(row_used[col] ? 1 : 0);
      if (row_used[col])
      {
        series[col].idx++;
      }
    }

    for (size_t col = 0; col < S; col++)
    {
      table.string_cols[col].push_back(str_row_used[col] ? str_row_values[col] : std::string());
      table.string_has_value[col].push_back(str_row_used[col] ? 1 : 0);
      if (str_row_used[col])
      {
        str_series[col].idx++;
      }
    }
  }

  return table;
}

// Serialize ExportTable to a plain CSV file.
// Layout: first column "time", then numeric columns, then string columns.
bool ToolboxCSV::serializeCSV(const ToolboxCSV::ExportTable& export_table, const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    return false;
  }

  QTextStream out(&file);
  out.setCodec("UTF-8");

  // Header: time, numeric columns, string columns.
  out << "time";
  for (const auto& col_name : export_table.names)
  {
    out << "," << QString::fromStdString(col_name);
  }
  for (const auto& col_name : export_table.string_names)
  {
    out << "," << QString::fromStdString(col_name);
  }
  out << "\n";

  const int time_decimals = 6;
  const int val_sig = 12;

  const int num_rows = static_cast<int>(export_table.time.size());
  const int num_cols = static_cast<int>(export_table.names.size());
  const int num_str_cols = static_cast<int>(export_table.string_names.size());

  for (int row = 0; row < num_rows; row++)
  {
    out << QString::number(export_table.time[row], 'f', time_decimals);

    // Numeric columns.
    for (int col = 0; col < num_cols; col++)
    {
      out << ",";
      if (export_table.has_value[col][row] == 0)
      {
        // Missing sample -> empty cell
      }
      else
      {
        const double val = export_table.cols[col][row];
        if (std::isnan(val))
        {
          out << "NaN";
        }
        else if (std::isfinite(val))
        {
          out << QString::number(val, 'g', val_sig);
        }
        else
        {
          out << "Inf";
        }
      }
    }

    // String columns with RFC 4180 quoting.
    for (int col = 0; col < num_str_cols; col++)
    {
      out << ",";
      if (export_table.string_has_value[col][row] != 0)
      {
        const QString val = QString::fromStdString(export_table.string_cols[col][row]);
        if (val.contains(',') || val.contains('"') || val.contains('\n') || val.contains('\r'))
        {
          QString escaped = val;
          escaped.replace('"', "\"\"");
          out << '"' << escaped << '"';
        }
        else
        {
          out << val;
        }
      }
    }

    out << "\n";
  }

  return true;
}

#if TOOLBOXCSV_WITH_PARQUET
// Build an Arrow Float64 array from a std::vector<double>.
// NULL represents missing sample; NaN/Inf preserved as Float64 values.
static arrow::Result<std::shared_ptr<arrow::Array>>
makeDoubleArray(const std::vector<double>& values, const std::vector<uint8_t>& present)
{
  arrow::DoubleBuilder builder;
  ARROW_RETURN_NOT_OK(builder.Reserve(static_cast<int64_t>(values.size())));

  if (present.size() != values.size())
  {
    return arrow::Status::Invalid("size mismatch");
  }

  for (size_t idx = 0; idx < values.size(); idx++)
  {
    if (!present[idx])
    {
      ARROW_RETURN_NOT_OK(builder.AppendNull());  // No sample -> Null
      continue;
    }

    const double val = values[idx];
    ARROW_RETURN_NOT_OK(builder.Append(val));  // Present sample (may be NaN/Inf)
  }

  std::shared_ptr<arrow::Array> arr;
  ARROW_RETURN_NOT_OK(builder.Finish(&arr));
  return arr;
}

// Build an Arrow UTF-8 string array. NULL represents missing sample.
static arrow::Result<std::shared_ptr<arrow::Array>>
makeStringArray(const std::vector<std::string>& values, const std::vector<uint8_t>& present)
{
  arrow::StringBuilder builder;
  ARROW_RETURN_NOT_OK(builder.Reserve(static_cast<int64_t>(values.size())));

  if (present.size() != values.size())
  {
    return arrow::Status::Invalid("size mismatch");
  }

  for (size_t idx = 0; idx < values.size(); idx++)
  {
    if (!present[idx])
    {
      ARROW_RETURN_NOT_OK(builder.AppendNull());
      continue;
    }
    ARROW_RETURN_NOT_OK(builder.Append(values[idx]));
  }

  std::shared_ptr<arrow::Array> arr;
  ARROW_RETURN_NOT_OK(builder.Finish(&arr));
  return arr;
}

// Serialize the merged ExportTable to a Parquet file using Arrow.
// Numeric columns as Float64, string columns as UTF-8.
bool ToolboxCSV::serializeParquet(const ToolboxCSV::ExportTable& export_table, const QString& path)
{
  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> arrays;

  fields.push_back(arrow::field("time", arrow::float64()));

  auto time_present = std::vector<uint8_t>(export_table.time.size(), 1);
  auto time_arr_res = makeDoubleArray(export_table.time, time_present);
  if (!time_arr_res.ok())
  {
    return false;
  }
  arrays.push_back(*time_arr_res);

  for (size_t col = 0; col < export_table.names.size(); col++)
  {
    fields.push_back(arrow::field(export_table.names[col], arrow::float64()));
    auto arr_res = makeDoubleArray(export_table.cols[col], export_table.has_value[col]);
    if (!arr_res.ok())
    {
      return false;
    }
    arrays.push_back(*arr_res);
  }

  // String columns as UTF-8.
  for (size_t col = 0; col < export_table.string_names.size(); col++)
  {
    fields.push_back(arrow::field(export_table.string_names[col], arrow::utf8()));
    auto arr_res =
        makeStringArray(export_table.string_cols[col], export_table.string_has_value[col]);
    if (!arr_res.ok())
    {
      return false;
    }
    arrays.push_back(*arr_res);
  }

  auto schema = std::make_shared<arrow::Schema>(fields);
  auto arrow_table = arrow::Table::Make(schema, arrays);

  auto outfile_res = arrow::io::FileOutputStream::Open(path.toStdString());
  if (!outfile_res.ok())
  {
    return false;
  }
  std::shared_ptr<arrow::io::FileOutputStream> outfile = *outfile_res;

  parquet::WriterProperties::Builder pb;
  std::shared_ptr<parquet::WriterProperties> props = pb.build();

  auto st = parquet::arrow::WriteTable(*arrow_table, arrow::default_memory_pool(), outfile,
                                       1024 * 64, props);
  if (!st.ok())
  {
    return false;
  }

  return true;
}
#endif
