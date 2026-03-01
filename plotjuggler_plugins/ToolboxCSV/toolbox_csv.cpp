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

  // Pick output file.
  connect(&_ui, &ToolBoxUI::pickFileRequested, this, &ToolboxCSV::on_toolButton_clicked);

  // Exporter.
  connect(&_ui, &ToolBoxUI::saveRequested, this, &ToolboxCSV::saveAll);
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

void ToolboxCSV::saveAll()
{
  // Convert relative time range to absolute time if needed.
  double t_start = _ui.getStartTime();
  double t_end = _ui.getEndTime();

  if (_ui.isRelativeTime())
  {
    t_start += _ui.getRelativeTime();
    t_end += _ui.getRelativeTime();
  }

  std::vector<std::string> selected_topics = _ui.getSelectedTopics();

  // Validate output path.
  const QString path = _ui.getPath();
  if (path.isEmpty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "Please select a file path before saving.");
    return;
  }

  const bool do_split = _ui.isCheckBoxTime();
  const double gap_sec = 10000.0;

  // Optionally split export into segments separated by large time gaps.
  auto ranges = getGlobalRangesByGap(selected_topics, t_start, t_end, gap_sec);

  if (!do_split || ranges.size() <= 1)
  {
    ExportTable t = buildExportTable(selected_topics, t_start, t_end);
    if (t.time.empty())
    {
      QMessageBox::warning(_ui.widget(), "Export", "No samples found in the selected time range.");
      return;
    }

    bool ok = false;
    if (_ui.isCsvButton())
    {
      ok = serializeCSV(t, path);
    }
    else
    {
#if TOOLBOXCSV_WITH_PARQUET
      ok = serializeParquet(t, path);
#else
      QMessageBox::warning(_ui.widget(), "Arrow/Parquet", "Can't load Arrow/Parquet");
      return;
#endif
    }

    if (!ok)
    {
      QMessageBox::warning(_ui.widget(), "Export", "Failed to write the output file.");
      return;
    }

    emit this->closed();
    return;
  }

  if (ranges.empty())
  {
    QMessageBox::warning(_ui.widget(), "Export", "No samples found in the selected time range.");
    return;
  }

  int parts_written = 0;

  for (int k = 0; k < static_cast<int>(ranges.size()); k++)
  {
    const double a = ranges[k].first;
    const double b = ranges[k].second;

    ExportTable t = buildExportTable(selected_topics, a, b);
    if (t.time.empty())
    {
      continue;
    }

    const QString out_path = addPartSuffix(path, k + 1, static_cast<int>(ranges.size()));

    bool ok = false;
    _ui.updateTimeControlsEnabled();
    if (_ui.isCsvButton())
    {
      ok = serializeCSV(t, out_path);
    }
    else
    {
#if TOOLBOXCSV_WITH_PARQUET
      ok = serializeParquet(t, out_path);
#else
      QMessageBox::warning(_ui.widget(), "Arrow/Parquet", "Can't load Arrow/Parquet");
      return;
#endif
    }

    if (!ok)
    {
      QMessageBox::warning(_ui.widget(), "Export", "Failed to write one of the output files.");
      return;
    }

    parts_written++;
  }

  if (parts_written == 0)
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

void ToolboxCSV::on_toolButton_clicked()
{
  // Persist last used file/dir per format.
  QSettings settings;

  const bool is_csv = _ui.isCsvButton();
  const QString fmt_key = is_csv ? "csv" : "parquet";

  const QString last_file =
      settings.value(QString("Export.%1.lastFile").arg(fmt_key), QString()).toString();

  const QString start_dir = settings.value("Export.lastDirectory", QDir::homePath()).toString();

  const QString default_name = is_csv ? "export.csv" : "export.parquet";

  const QString start_path =
      last_file.isEmpty() ? QDir(start_dir).filePath(default_name) : last_file;

  QFileDialog dialog(_ui.widget());
  dialog.setAcceptMode(QFileDialog::AcceptSave);
  dialog.setFileMode(QFileDialog::AnyFile);

  if (is_csv)
  {
    dialog.setNameFilter("CSV (*.csv)");
    dialog.setDefaultSuffix("csv");
  }
  else
  {
    dialog.setNameFilter("Parquet (*.parquet *.pq)");
    dialog.setDefaultSuffix("parquet");
  }

  dialog.selectFile(start_path);
  dialog.setOption(QFileDialog::DontUseNativeDialog, true);

  if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
  {
    return;
  }

  const QString file_path = dialog.selectedFiles().first();
  if (file_path.isEmpty())
  {
    return;
  }

  const QFileInfo info(file_path);

  settings.setValue(QString("Export.%1.lastFile").arg(fmt_key), file_path);
  settings.setValue("Export.lastDirectory", info.absolutePath());

  _ui.setPath(file_path);
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

    const double a = plot.front().x;
    const double b = plot.back().x;

    if (!any)
    {
      tmin = a;
      tmax = b;
      any = true;
    }
    else
    {
      tmin = std::min(tmin, a);
      tmax = std::max(tmax, b);
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

  for (size_t i = start_idx + 1; i <= last; i++)
  {
    const double t0 = plot.at(i - 1).x;
    const double t1 = plot.at(i).x;
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
    for (const auto& s : series)
    {
      double dt = estimateMinDt(*s.plot, s.idx, t_end);
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
  for (const auto& s : series)
  {
    table.names.push_back(s.name);
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

    for (size_t i = 0; i < N; i++)
    {
      auto& s = series[i];
      if (s.idx >= s.plot->size())
      {
        continue;
      }

      const auto& p = s.plot->at(s.idx);
      if (p.x > t_end)
      {
        continue;
      }

      done = false;
      if (p.x < min_time)
      {
        min_time = p.x;
      }
    }

    if (done || min_time > t_end)
    {
      break;
    }

    std::fill(row_values.begin(), row_values.end(), NaN);
    std::fill(row_used.begin(), row_used.end(), false);

    // Fill values for this row (within tolerance), then advance only the series that contributed.
    for (size_t i = 0; i < N; i++)
    {
      auto& s = series[i];
      if (s.idx >= s.plot->size())
      {
        continue;
      }

      const auto& p = s.plot->at(s.idx);
      if (p.x > t_end)
      {
        continue;
      }

      if (tol == 0.0)
      {
        if (p.x == min_time)
        {
          row_values[i] = p.y;
          row_used[i] = true;
        }
      }
      else
      {
        if (std::abs(p.x - min_time) <= tol)
        {
          row_values[i] = p.y;
          row_used[i] = true;
        }
      }
    }

    table.time.push_back(min_time);
    for (size_t i = 0; i < N; i++)
    {
      table.cols[i].push_back(row_values[i]);
      table.has_value[i].push_back(row_used[i] ? 1 : 0);
      if (row_used[i])
      {
        series[i].idx++;
      }
    }
  }

  return table;
}

// Serialize ExportTable to a plain CSV file.
// Layout: first column "time", followed by one column per topic.
bool ToolboxCSV::serializeCSV(const ToolboxCSV::ExportTable& t, const QString& path)
{
  // Write CSV: empty cell for missing samples; explicit NaN/Inf when present.
  if (path.isEmpty())
  {
    return false;
  }
  if (t.time.empty())
  {
    return false;
  }
  if (t.cols.size() != t.names.size())
  {
    return false;
  }

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    return false;
  }

  QTextStream out(&f);
  out.setCodec("UTF-8");

  out << "time";
  for (const auto& n : t.names)
  {
    out << "," << QString::fromStdString(n);
  }
  out << "\n";

  const int time_decimals = 6;
  const int val_sig = 12;

  const int rows = static_cast<int>(t.time.size());
  const int cols = static_cast<int>(t.names.size());

  for (int r = 0; r < rows; r++)
  {
    out << QString::number(t.time[r], 'f', time_decimals);

    for (int c = 0; c < cols; c++)
    {
      out << ",";
      if (t.has_value[c][r] == 0)
      {
        // Missing sample -> empty cell
      }
      else
      {
        const double v = t.cols[c][r];
        if (std::isnan(v))
        {
          out << "NaN";
        }
        else if (std::isfinite(v))
        {
          out << QString::number(v, 'g', val_sig);
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
makeDoubleArray(const std::vector<double>& v, const std::vector<uint8_t>& present)
{
  arrow::DoubleBuilder b;
  ARROW_RETURN_NOT_OK(b.Reserve(static_cast<int64_t>(v.size())));

  if (present.size() != v.size())
  {
    return arrow::Status::Invalid("size mismatch");
  }

  for (size_t i = 0; i < v.size(); i++)
  {
    if (!present[i])
    {
      ARROW_RETURN_NOT_OK(b.AppendNull());  // No sample -> Null
      continue;
    }

    const double x = v[i];
    ARROW_RETURN_NOT_OK(b.Append(x));  // Present sample (may be NaN/Inf)
  }

  std::shared_ptr<arrow::Array> arr;
  ARROW_RETURN_NOT_OK(b.Finish(&arr));
  return arr;
}

// Serialize the merged ExportTable to a Parquet file using Arrow.
// Each column is stored as Float64.
bool ToolboxCSV::serializeParquet(const ToolboxCSV::ExportTable& t, const QString& path)
{
  if (path.isEmpty())
  {
    return false;
  }
  if (t.time.empty())
  {
    return false;
  }
  if (t.cols.size() != t.names.size())
  {
    return false;
  }

  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> arrays;

  fields.push_back(arrow::field("time", arrow::float64()));

  auto time_present = std::vector<uint8_t>(t.time.size(), 1);
  auto time_arr_res = makeDoubleArray(t.time, time_present);
  if (!time_arr_res.ok())
  {
    return false;
  }
  arrays.push_back(*time_arr_res);

  for (size_t i = 0; i < t.names.size(); i++)
  {
    fields.push_back(arrow::field(t.names[i], arrow::float64()));
    auto arr_res = makeDoubleArray(t.cols[i], t.has_value[i]);
    if (!arr_res.ok())
    {
      return false;
    }
    arrays.push_back(*arr_res);
  }

  auto schema = std::make_shared<arrow::Schema>(fields);
  auto table = arrow::Table::Make(schema, arrays);

  auto outfile_res = arrow::io::FileOutputStream::Open(path.toStdString());
  if (!outfile_res.ok())
  {
    return false;
  }
  std::shared_ptr<arrow::io::FileOutputStream> outfile = *outfile_res;

  parquet::WriterProperties::Builder pb;
  std::shared_ptr<parquet::WriterProperties> props = pb.build();

  auto st =
      parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024 * 64, props);
  if (!st.ok())
  {
    return false;
  }

  return true;
}
#endif

// Detect continuous time segments separated by gaps larger than gap_sec.
std::vector<std::pair<double, double>> ToolboxCSV::getGlobalRangesByGap(
    const std::vector<std::string>& topics, double t_start, double t_end, double gap_sec) const
{
  struct S
  {
    const PJ::PlotData* p;
    size_t i;
  };

  std::vector<S> s;
  s.reserve(topics.size());

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

    int idx = plot.getIndexFromX(t_start);
    if (idx < 0)
    {
      continue;
    }

    s.push_back({ &plot, static_cast<size_t>(idx) });
  }

  if (s.empty())
  {
    return {};
  }

  std::vector<std::pair<double, double>> ranges;

  bool started = false;
  double seg_a = 0.0, seg_b = 0.0;
  double prev = 0.0;

  while (true)
  {
    bool done = true;
    double min_time = std::numeric_limits<double>::max();

    for (auto& x : s)
    {
      if (x.i >= x.p->size())
      {
        continue;
      }
      const double t = x.p->at(x.i).x;
      if (t > t_end)
      {
        continue;
      }
      done = false;
      if (t < min_time)
      {
        min_time = t;
      }
    }

    if (done || min_time > t_end)
    {
      break;
    }

    if (!started)
    {
      seg_a = min_time;
      seg_b = min_time;
      prev = min_time;
      started = true;
    }
    else
    {
      if (min_time - prev > gap_sec)
      {
        ranges.push_back({ seg_a, seg_b });
        seg_a = min_time;
        seg_b = min_time;
      }
      else
      {
        seg_b = min_time;
      }
      prev = min_time;
    }

    for (auto& x : s)
    {
      if (x.i >= x.p->size())
      {
        continue;
      }
      const double t = x.p->at(x.i).x;
      if (t > t_end)
      {
        continue;
      }
      if (t == min_time)
      {
        x.i++;
      }
    }
  }

  if (started)
  {
    ranges.push_back({ seg_a, seg_b });
  }
  return ranges;
}

// Append "_partXX" suffix while preserving original file extension.
QString ToolboxCSV::addPartSuffix(const QString& path, int part_idx, int parts_total)
{
  QFileInfo info(path);
  const QString base = info.completeBaseName();
  const QString ext = info.suffix();
  const QString dir = info.absolutePath();

  const int width = (parts_total >= 100) ? 3 : 2;
  const QString suffix = QString("_part%1").arg(part_idx, width, 10, QChar('0'));

  const QString filename = ext.isEmpty() ? (base + suffix) : (base + suffix + "." + ext);
  return QDir(dir).filePath(filename);
}
