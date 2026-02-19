#include "toolbox_csv.h"
#include "PlotJuggler/svg_util.h"
#include "ui_toolbox_csv.h"

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

ToolboxCSV::ToolboxCSV()
{
  _widget = new QWidget();
  ui = new Ui::toolBoxUI();

  ui->setupUi(_widget);

  // Enable drag & drop into the table.
  ui->tableWidget->installEventFilter(this);
  ui->tableWidget->viewport()->installEventFilter(this);

  // Close/cancel
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolboxCSV::onClosed);

  // Dummy initial range; actual range recomputed from selected topics.
  ui->rangeSlider->setOptions(RangeSlider::DoubleHandles);
  ui->rangeSlider->setRangeReal(0.0, 1.0, 2);
  ui->rangeSlider->setShowTicks(false);

  ui->startTime->setRange(0.0, 1.0);
  ui->startTime->setDecimals(2);
  ui->startTime->setButtonSymbols(QAbstractSpinBox::NoButtons);

  ui->endTime->setRange(0.0, 1.0);
  ui->endTime->setValue(1.0);
  ui->endTime->setDecimals(2);
  ui->endTime->setButtonSymbols(QAbstractSpinBox::NoButtons);

  // Default export mode.
  ui->csvButton->setChecked(true);
  ui->relativeBox->setChecked(true);

  // Slider -> SpinBox sync (prevent feedback loops).
  connect(ui->rangeSlider, &RangeSlider::lowerValueChanged, _widget, [this](int v) {
    QSignalBlocker b(*ui->startTime);
    ui->startTime->setValue(ui->rangeSlider->toReal(v));
  });

  connect(ui->rangeSlider, &RangeSlider::upperValueChanged, _widget, [this](int v) {
    QSignalBlocker b(*ui->endTime);
    ui->endTime->setValue(ui->rangeSlider->toReal(v));
  });

  // SpinBox -> Slider sync (prevent recursive updates).
  connect(ui->startTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), _widget,
          [this](double t) {
            QSignalBlocker b(*ui->rangeSlider);
            ui->rangeSlider->setLowerValue(ui->rangeSlider->toInt(t));
          });

  connect(ui->endTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), _widget,
          [this](double t) {
            QSignalBlocker b(*ui->rangeSlider);
            ui->rangeSlider->setUpperValue(ui->rangeSlider->toInt(t));
          });

  // Remove selected rows bottom-up to keep indices valid.
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
    updateTimeRange();
  });

  // Clear all topics.
  connect(ui->clearButton, &QToolButton::clicked, _widget, [this]() {
    ui->tableWidget->setRowCount(0);
    updateTimeControlsEnabled();
    updateTimeRange();
  });

  // Pick output file.
  connect(ui->toolButton, &QToolButton::clicked, this, &ToolboxCSV::on_toolButton_clicked);

  // Reset output path when switching format.
  connect(ui->csvButton, &QRadioButton::toggled, _widget, [this](bool checked) {
    if (!checked)
      return;
    ui->lineEditPath->clear();
  });

  connect(ui->parquetButton, &QRadioButton::toggled, _widget, [this](bool checked) {
    if (!checked)
      return;
    ui->lineEditPath->clear();
  });

  // Recompute visible time range when switching relative/absolute.
  connect(ui->relativeBox, &QCheckBox::toggled, this, &ToolboxCSV::updateTimeRange);

  // Exporter.
  connect(ui->saveButton, &QPushButton::clicked, this, &ToolboxCSV::saveAll);
}

ToolboxCSV::~ToolboxCSV()
{
  delete ui;
  delete _widget;
}

void ToolboxCSV::init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxCSV::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::WidgetType::FIXED };
}

bool ToolboxCSV::onShowWidget()
{
  // Load icons based on current theme.
  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();

  ui->clearButton->setIcon(LoadSvg(":/resources/svg/clear.svg", theme));
  ui->saveButton->setIcon(LoadSvg(":/resources/svg/save.svg", theme));

  // Small UI polish.
  auto* corner = ui->tableWidget->findChild<QAbstractButton*>();
  if (corner)
  {
    corner->setToolTip("Click to select all topics");
  }

  ui->tableWidget->setStyleSheet("QTableCornerButton::section {"
                                 "  background-color: palette(button);"
                                 "  border: 1px solid palette(mid);"
                                 "}");

  ui->tableWidget->horizontalHeader()->setStyleSheet("QHeaderView::section { font-weight: bold; }");

  updateTimeControlsEnabled();

  // Recompute time range on show in case selected topics changed.
  updateTimeRange();

  return true;
}

bool ToolboxCSV::eventFilter(QObject* obj, QEvent* ev)
{
  if (ev->type() == QEvent::DragEnter)
  {
    // Accept PlotJuggler curve drags onto the table.
    auto* event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    const QStringList mimeFormats = mimeData->formats();

    // Parse PlotJuggler curve list from drag MIME payload.
    for (const QString& format : mimeFormats)
    {
      if (format != "curveslist/add_curve")
      {
        continue;
      }

      QByteArray encoded = mimeData->data(format);
      QDataStream stream(&encoded, QIODevice::ReadOnly);

      QStringList curves;
      while (!stream.atEnd())
      {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty())
        {
          curves.push_back(curve_name);
        }
      }

      if (curves.isEmpty())
      {
        return false;
      }

      _dragging_curves = curves;

      if (obj == ui->tableWidget || obj == ui->tableWidget->viewport())
      {
        event->acceptProposedAction();
        return true;
      }
    }

    return false;
  }
  else if (ev->type() == QEvent::DragMove)
  {
    auto* event = static_cast<QDragMoveEvent*>(ev);
    if (obj == ui->tableWidget || obj == ui->tableWidget->viewport())
    {
      if (event->mimeData() && event->mimeData()->hasFormat("curveslist/add_curve"))
      {
        event->acceptProposedAction();
        return true;
      }
    }
    return false;
  }
  else if (ev->type() == QEvent::Drop)
  {
    auto* event = static_cast<QDropEvent*>(ev);

    if (obj != ui->tableWidget && obj != ui->tableWidget->viewport())
    {
      return false;
    }

    QSet<QString> existing;
    existing.reserve(ui->tableWidget->rowCount());

    for (int r = 0; r < ui->tableWidget->rowCount(); r++)
    {
      auto* item = ui->tableWidget->item(r, 0);
      if (item)
      {
        const QString t = item->text().trimmed();
        if (!t.isEmpty())
          existing.insert(t);
      }
    }

    for (const QString& raw : _dragging_curves)
    {
      const QString topic = raw.trimmed();
      if (topic.isEmpty())
        continue;

      if (existing.contains(topic))
        continue;

      existing.insert(topic);

      const int row = ui->tableWidget->rowCount();
      ui->tableWidget->insertRow(row);
      ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
    }

    _dragging_curves.clear();
    updateTimeControlsEnabled();
    updateTimeRange();
    event->acceptProposedAction();
    return true;
  }

  return false;
}

void ToolboxCSV::saveAll()
{
  // Convert relative time range to absolute time if needed.
  double t_start = ui->startTime->value();
  double t_end = ui->endTime->value();

  if (ui->relativeBox->isChecked())
  {
    t_start += _t0;
    t_end += _t0;
  }

  // Collect selected topic names from the UI table.
  std::vector<std::string> selected_topics;
  for (int r = 0; r < ui->tableWidget->rowCount(); r++)
  {
    auto* item = ui->tableWidget->item(r, 0);
    if (!item)
      continue;

    const std::string name = item->text().trimmed().toStdString();
    if (!name.empty())
    {
      selected_topics.push_back(name);
    }
  }

  // Validate output path.
  const QString path = ui->lineEditPath->text().trimmed();
  if (path.isEmpty())
  {
    QMessageBox::warning(_widget, "Export", "Please select a file path before saving.");
    return;
  }

  const bool do_split = ui->checkBoxTime->isChecked();
  const double gap_sec = 10000.0;

  // Optionally split export into segments separated by large time gaps.
  auto ranges = getGlobalRangesByGap(selected_topics, t_start, t_end, gap_sec);

  if (!do_split || ranges.size() <= 1)
  {
    ExportTable t = buildExportTable(selected_topics, t_start, t_end);
    if (t.time.empty())
    {
      QMessageBox::warning(_widget, "Export", "No samples found in the selected time range.");
      return;
    }

    bool ok = false;
    if (ui->csvButton->isChecked())
    {
      ok = serializeCSV(t, path);
    }
    else
    {
#if TOOLBOXCSV_WITH_PARQUET
      ok = serializeParquet(t, path);
#else
      QMessageBox::warning(_widget, "Arrow/Parquet", "Can't load Arrow/Parquet");
      return;
#endif
    }

    if (!ok)
    {
      QMessageBox::warning(_widget, "Export", "Failed to write the output file.");
      return;
    }

    emit this->closed();
    return;
  }

  if (ranges.empty())
  {
    QMessageBox::warning(_widget, "Export", "No samples found in the selected time range.");
    return;
  }

  int parts_written = 0;

  for (int k = 0; k < static_cast<int>(ranges.size()); k++)
  {
    const double a = ranges[k].first;
    const double b = ranges[k].second;

    ExportTable t = buildExportTable(selected_topics, a, b);
    if (t.time.empty())
      continue;

    const QString out_path = addPartSuffix(path, k + 1, static_cast<int>(ranges.size()));

    bool ok = false;
    if (ui->csvButton->isChecked())
    {
      ok = serializeCSV(t, out_path);
    }
    else
    {
#if TOOLBOXCSV_WITH_PARQUET
      ok = serializeParquet(t, out_path);
#else
      QMessageBox::warning(_widget, "Arrow/Parquet", "Can't load Arrow/Parquet");
      return;
#endif
    }

    if (!ok)
    {
      QMessageBox::warning(_widget, "Export", "Failed to write one of the output files.");
      return;
    }

    parts_written++;
  }

  if (parts_written == 0)
  {
    QMessageBox::warning(_widget, "Export", "No samples found in the selected time range.");
    return;
  }

  emit this->closed();
}

void ToolboxCSV::onClosed()
{
  emit this->closed();
}

void ToolboxCSV::on_toolButton_clicked()
{
  // Persist last used file/dir per format.
  QSettings settings;

  const bool is_csv = ui->csvButton->isChecked();
  const QString fmt_key = is_csv ? "csv" : "parquet";

  const QString last_file =
      settings.value(QString("Export.%1.lastFile").arg(fmt_key), QString()).toString();

  const QString start_dir = settings.value("Export.lastDirectory", QDir::homePath()).toString();

  const QString default_name = is_csv ? "export.csv" : "export.parquet";

  const QString start_path =
      last_file.isEmpty() ? QDir(start_dir).filePath(default_name) : last_file;

  QFileDialog dialog(_widget);
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

  ui->lineEditPath->setText(file_path);
}

// Disable time and export controls when no topics are selected.
void ToolboxCSV::updateTimeControlsEnabled()
{
  const bool has_data = ui->tableWidget->rowCount() > 0;
  ui->startTime->setEnabled(has_data);
  ui->endTime->setEnabled(has_data);
  ui->rangeSlider->setEnabled(has_data);
  ui->toolButton->setEnabled(has_data);
  ui->saveButton->setEnabled(has_data);
}

// Compute global [tmin, tmax] across selected numeric topics.
bool ToolboxCSV::getTimeRange(double& tmin, double& tmax) const
{
  bool any = false;

  for (int r = 0; r < ui->tableWidget->rowCount(); r++)
  {
    auto* item = ui->tableWidget->item(r, 0);
    if (!item)
      continue;

    const std::string name = item->text().trimmed().toStdString();
    if (name.empty())
      continue;

    auto it = _plot_data->numeric.find(name);
    if (it == _plot_data->numeric.end())
      continue;

    const auto& plot = it->second;
    if (plot.size() == 0)
      continue;

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
    return false;
  if (tmax < tmin)
    std::swap(tmin, tmax);
  return true;
}

// Update UI time range in relative or absolute mode.
void ToolboxCSV::setTimeRange(double tmin, double tmax)
{
  // Update slider/spinboxes with either relative time (starting at 0) or absolute time.
  const bool relative = ui->relativeBox->isChecked();
  const int decimals = 3;

  QSignalBlocker b0(*ui->rangeSlider);
  QSignalBlocker b1(*ui->startTime);
  QSignalBlocker b2(*ui->endTime);

  if (relative)
  {
    _t0 = tmin;
    double duration = tmax - tmin;
    if (duration < 0.0)
      duration = 0.0;

    ui->rangeSlider->setRangeReal(0.0, duration, decimals);

    ui->startTime->setDecimals(decimals);
    ui->startTime->setRange(0.0, duration);
    ui->startTime->setValue(0.0);

    ui->endTime->setDecimals(decimals);
    ui->endTime->setRange(0.0, duration);
    ui->endTime->setValue(duration);

    ui->rangeSlider->setLowerValue(ui->rangeSlider->toInt(0.0));
    ui->rangeSlider->setUpperValue(ui->rangeSlider->toInt(duration));
  }
  else
  {
    _t0 = 0.0;

    ui->rangeSlider->setRangeReal(tmin, tmax, decimals);

    ui->startTime->setDecimals(decimals);
    ui->startTime->setRange(tmin, tmax);
    ui->startTime->setValue(tmin);

    ui->endTime->setDecimals(decimals);
    ui->endTime->setRange(tmin, tmax);
    ui->endTime->setValue(tmax);

    ui->rangeSlider->setLowerValue(ui->rangeSlider->toInt(tmin));
    ui->rangeSlider->setUpperValue(ui->rangeSlider->toInt(tmax));
  }
}

void ToolboxCSV::updateTimeRange()
{
  double tmin, tmax;
  if (!getTimeRange(tmin, tmax))
    return;  // No valid topics -> keep current UI range
  setTimeRange(tmin, tmax);
}

// Estimate local minimum dt to derive adaptive merge tolerance.
double ToolboxCSV::estimateMinDt(const PJ::PlotData& plot, size_t start_idx, double t_end)
{
  if (plot.size() < 2 || start_idx + 1 >= plot.size())
    return 0.0;

  double min_dt = std::numeric_limits<double>::max();
  size_t last = std::min(plot.size() - 1, start_idx + 2000);

  for (size_t i = start_idx + 1; i <= last; i++)
  {
    const double t0 = plot.at(i - 1).x;
    const double t1 = plot.at(i).x;
    if (t0 > t_end)
      break;
    const double dt = t1 - t0;
    if (dt > 0.0 && dt < min_dt)
      min_dt = dt;
  }

  if (min_dt == std::numeric_limits<double>::max())
    return 0.0;
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
      continue;

    const auto& plot = it->second;
    if (plot.size() == 0)
      continue;
    if (plot.front().x > t_end)
      continue;
    if (plot.back().x < t_start)
      continue;

    int index = plot.getIndexFromX(t_start);
    if (index < 0)
      continue;

    series.push_back({ name, &plot, static_cast<size_t>(index) });
  }

  if (series.empty())
    return table;

  // Compute tolerance from the minimum observed sample period.
  double min_dt = 0.0;
  {
    double best = std::numeric_limits<double>::max();
    for (const auto& s : series)
    {
      double dt = estimateMinDt(*s.plot, s.idx, t_end);
      if (dt > 0.0 && dt < best)
        best = dt;
    }
    if (best != std::numeric_limits<double>::max())
      min_dt = best;
  }

  // Tolerance = 0.5 * min_dt to avoid merging consecutive distinct samples.
  const double tol = (min_dt > 0.0) ? (0.5 * min_dt) : 0.0;

  const size_t N = series.size();
  table.names.reserve(N);
  table.cols.assign(N, {});
  table.has_value.assign(N, {});
  for (const auto& s : series)
    table.names.push_back(s.name);

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
        continue;

      const auto& p = s.plot->at(s.idx);
      if (p.x > t_end)
        continue;

      done = false;
      if (p.x < min_time)
        min_time = p.x;
    }

    if (done || min_time > t_end)
      break;

    std::fill(row_values.begin(), row_values.end(), NaN);
    std::fill(row_used.begin(), row_used.end(), false);

    // Fill values for this row (within tolerance), then advance only the series that contributed.
    for (size_t i = 0; i < N; i++)
    {
      auto& s = series[i];
      if (s.idx >= s.plot->size())
        continue;

      const auto& p = s.plot->at(s.idx);
      if (p.x > t_end)
        continue;

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
        series[i].idx++;
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
    return false;
  if (t.time.empty())
    return false;
  if (t.cols.size() != t.names.size())
    return false;

  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return false;

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
          out << "NaN";
        else if (std::isfinite(v))
          out << QString::number(v, 'g', val_sig);
        else
          out << "Inf";
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
    return arrow::Status::Invalid("size mismatch");

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
    return false;
  if (t.time.empty())
    return false;
  if (t.cols.size() != t.names.size())
    return false;

  std::vector<std::shared_ptr<arrow::Field>> fields;
  std::vector<std::shared_ptr<arrow::Array>> arrays;

  fields.push_back(arrow::field("time", arrow::float64()));

  auto time_present = std::vector<uint8_t>(t.time.size(), 1);
  auto time_arr_res = makeDoubleArray(t.time, time_present);
  if (!time_arr_res.ok())
    return false;
  arrays.push_back(*time_arr_res);

  for (size_t i = 0; i < t.names.size(); i++)
  {
    fields.push_back(arrow::field(t.names[i], arrow::float64()));
    auto arr_res = makeDoubleArray(t.cols[i], t.has_value[i]);
    if (!arr_res.ok())
      return false;
    arrays.push_back(*arr_res);
  }

  auto schema = std::make_shared<arrow::Schema>(fields);
  auto table = arrow::Table::Make(schema, arrays);

  auto outfile_res = arrow::io::FileOutputStream::Open(path.toStdString());
  if (!outfile_res.ok())
    return false;
  std::shared_ptr<arrow::io::FileOutputStream> outfile = *outfile_res;

  parquet::WriterProperties::Builder pb;
  std::shared_ptr<parquet::WriterProperties> props = pb.build();

  auto st =
      parquet::arrow::WriteTable(*table, arrow::default_memory_pool(), outfile, 1024 * 64, props);
  if (!st.ok())
    return false;

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
      continue;

    const auto& plot = it->second;
    if (plot.size() == 0)
      continue;
    if (plot.front().x > t_end)
      continue;
    if (plot.back().x < t_start)
      continue;

    int idx = plot.getIndexFromX(t_start);
    if (idx < 0)
      continue;

    s.push_back({ &plot, static_cast<size_t>(idx) });
  }

  if (s.empty())
    return {};

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
        continue;
      const double t = x.p->at(x.i).x;
      if (t > t_end)
        continue;
      done = false;
      if (t < min_time)
        min_time = t;
    }

    if (done || min_time > t_end)
      break;

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
        continue;
      const double t = x.p->at(x.i).x;
      if (t > t_end)
        continue;
      if (t == min_time)
        x.i++;
    }
  }

  if (started)
    ranges.push_back({ seg_a, seg_b });
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
