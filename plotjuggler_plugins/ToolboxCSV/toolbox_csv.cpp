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

  connect(ui->removeButton, &QToolButton::clicked, _widget, [this]() {
    auto* table = ui->tableWidget;
    QModelIndexList rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty())
    {
      return;
    }

    std::sort(rows.begin(), rows.end(),
              [](const QModelIndex& a, const QModelIndex& b) { return a.row() > b.row(); });

    for (const auto& idx : rows)
    {
      table->removeRow(idx.row());
    }

    updateTimeControlsEnabled();
  });

  connect(ui->clearButton, &QToolButton::clicked, _widget, [this]() {
    ui->tableWidget->setRowCount(0);
    updateTimeControlsEnabled();
  });

  return true;
}

bool ToolboxCSV::serializeTable(const ExportTable& table, const QString& path, bool is_csv)
{
  if (path.isEmpty() || table.time.empty() || table.cols.size() != table.names.size())
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

  // Group selected topics by their PlotGroup name.
  std::map<std::string, std::vector<std::string>> groups;

  for (const auto& topic_name : selected_topics)
  {
    auto it = _plot_data->numeric.find(topic_name);
    if (it == _plot_data->numeric.end())
    {
      continue;
    }

    const auto& group_ptr = it->second.group();
    std::string group_name = group_ptr ? group_ptr->name() : "ungrouped";
    groups[group_name].push_back(topic_name);
  }

  int files_written = 0;

  for (const auto& [group_name, topics] : groups)
  {
    ExportTable table = buildExportTable(topics, t_start, t_end);
    if (table.time.empty())
    {
      continue;
    }

    // Strip group prefix from column names (e.g. "/robot/imu/accel" -> "accel").
    for (auto& col_name : table.names)
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

    files_written++;
  }

  if (files_written == 0)
  {
    QMessageBox::warning(_ui.widget(), "Export", "No samples found in the selected time range.");
    return;
  }

  emit closed();
}

void ToolboxCSV::onClosed()
{
  emit closed();
}

// Compute global [tmin, tmax] across selected numeric topics.
bool ToolboxCSV::getTimeRange(double& tmin, double& tmax) const
{
  bool any = false;

  const auto topics = _ui.getSelectedTopics();

  for (const auto& name : topics)
  {
    auto it = _plot_data->numeric.find(name);
    if (it == _plot_data->numeric.end())
    {
      continue;
    }

    const auto& plot = it->second;
    if (plot.size() == 0)
    {
      continue;
    }

    const double front_time = plot.front().x;
    const double back_time = plot.back().x;

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
double ToolboxCSV::estimateMinDt(const PJ::PlotData& plot, size_t start_idx, double t_end)
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
    if (plot.size() == 0)
    {
      continue;
    }
    if (plot.front().x > t_end)
    {
      continue;
    }
    if (plot.back().x < t_start)
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

  if (series.empty())
  {
    return table;
  }

  // Compute tolerance from the minimum observed sample period.
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
    if (best != std::numeric_limits<double>::max())
    {
      min_dt = best;
    }
  }

  // Tolerance = 0.5 * min_dt to avoid merging consecutive distinct samples.
  const double tol = (min_dt > 0.0) ? (0.5 * min_dt) : 0.0;

  const size_t N = series.size();
  table.names.reserve(N);
  table.cols.assign(N, {});
  table.has_value.assign(N, {});
  for (const auto& sr : series)
  {
    table.names.push_back(sr.name);
  }

  const auto NaN = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> row_values(N, NaN);

  // Track sample presence separately from numeric value (handles NaN correctly).
  std::vector<bool> row_used(N, false);

  while (true)
  {
    // Find next row time as the minimum current timestamp among series.
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

    if (done || min_time > t_end)
    {
      break;
    }

    std::fill(row_values.begin(), row_values.end(), NaN);
    std::fill(row_used.begin(), row_used.end(), false);

    // Fill values for this row (within tolerance), then advance only the series that contributed.
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

      if (tol == 0.0)
      {
        if (point.x == min_time)
        {
          row_values[col] = point.y;
          row_used[col] = true;
        }
      }
      else
      {
        if (std::abs(point.x - min_time) <= tol)
        {
          row_values[col] = point.y;
          row_used[col] = true;
        }
      }
    }

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
  }

  return table;
}

// Serialize ExportTable to a plain CSV file.
// Layout: first column "time", followed by one column per topic.
bool ToolboxCSV::serializeCSV(const ToolboxCSV::ExportTable& export_table, const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    return false;
  }

  QTextStream out(&file);
  out.setCodec("UTF-8");

  out << "time";
  for (const auto& col_name : export_table.names)
  {
    out << "," << QString::fromStdString(col_name);
  }
  out << "\n";

  const int time_decimals = 6;
  const int val_sig = 12;

  const int num_rows = static_cast<int>(export_table.time.size());
  const int num_cols = static_cast<int>(export_table.names.size());

  for (int row = 0; row < num_rows; row++)
  {
    out << QString::number(export_table.time[row], 'f', time_decimals);

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

// Serialize the merged ExportTable to a Parquet file using Arrow.
// Each column is stored as Float64.
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
