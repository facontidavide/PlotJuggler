#include "toolbox_ui.h"
#include "ui_toolbox_csv.h"
#include "PlotJuggler/svg_util.h"

#include <QSignalBlocker>
#include <QAbstractSpinBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDataStream>
#include <QAbstractButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

#include <QSettings>

ToolBoxUI::ToolBoxUI()
{
  _widget = new QWidget();
  ui = new Ui::toolBoxUI();

  ui->setupUi(_widget);

  // Enable drag & drop into the table.
  ui->tableWidget->installEventFilter(this);
  ui->tableWidget->viewport()->installEventFilter(this);

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
  ui->comboTime->setCurrentIndex(0);  // relative

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

  // Slider -> SpinBox sync (prevent feedback loops).
  connect(ui->rangeSlider, &RangeSlider::lowerValueChanged, _widget, [this](int slider_val) {
    QSignalBlocker blocker(*ui->startTime);
    ui->startTime->setValue(ui->rangeSlider->toReal(slider_val));
  });

  connect(ui->rangeSlider, &RangeSlider::upperValueChanged, _widget, [this](int slider_val) {
    QSignalBlocker blocker(*ui->endTime);
    ui->endTime->setValue(ui->rangeSlider->toReal(slider_val));
  });

  // SpinBox -> Slider sync (prevent recursive updates).
  connect(ui->startTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), _widget,
          [this](double time_val) {
            QSignalBlocker blocker(*ui->rangeSlider);
            ui->rangeSlider->setLowerValue(ui->rangeSlider->toInt(time_val));
          });

  connect(ui->endTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), _widget,
          [this](double time_val) {
            QSignalBlocker blocker(*ui->rangeSlider);
            ui->rangeSlider->setUpperValue(ui->rangeSlider->toInt(time_val));
          });

  // Filter applicator in QtableWidget
  connect(ui->lineEditFilter, &QLineEdit::textChanged, this, [this](const QString& text) {
    const QString filter = text.trimmed();

    for (int row = 0; row < ui->tableWidget->rowCount(); row++)
    {
      auto* item = ui->tableWidget->item(row, 0);
      if (!item)
      {
        ui->tableWidget->setRowHidden(row, !filter.isEmpty());
        continue;
      }

      if (filter.isEmpty())
      {
        ui->tableWidget->setRowHidden(row, false);
        continue;
      }

      const bool match = item->text().contains(filter, Qt::CaseInsensitive);
      ui->tableWidget->setRowHidden(row, !match);
    }
  });

  connect(ui->checkBoxMultifile, &QCheckBox::toggled, this, [this](bool multi_file) {
    ui->lineEditPrefix->setHidden(!multi_file);
    ui->labelPrefix->setHidden(!multi_file);
  });

  connect(ui->removeButton, &QToolButton::clicked, this, &ToolBoxUI::removeRequested);
  connect(ui->clearButton, &QToolButton::clicked, this, &ToolBoxUI::clearRequested);
  connect(ui->buttonAddAllFiles, &QToolButton::clicked, this, &ToolBoxUI::addAllRequested);
  connect(ui->comboTime, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, [this](int) { recomputeTime(); });
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolBoxUI::closed);
  connect(ui->saveButton, &QPushButton::clicked, this, [this]() {
    const bool is_csv = ui->csvButton->isChecked();
    const bool multi_file = ui->checkBoxMultifile->isChecked();

    QSettings settings;
    const QString last_dir = settings.value("Export.lastDirectory", QDir::homePath()).toString();

    if (!multi_file)
    {
      const QString filter = is_csv ? "CSV (*.csv)" : "Parquet (*.parquet *.pq)";
      const QString suffix = is_csv ? "csv" : "parquet";
      const QString default_name =
          QDir(last_dir).filePath(is_csv ? "export.csv" : "export.parquet");

      QString filename = QFileDialog::getSaveFileName(_widget, "Export data", default_name, filter);
      if (filename.isEmpty())
      {
        return;
      }
      if (!filename.endsWith("." + suffix, Qt::CaseInsensitive))
      {
        filename += "." + suffix;
      }

      settings.setValue("Export.lastDirectory", QFileInfo(filename).absolutePath());
      emit exportSingleFile(is_csv, filename);
    }
    else
    {
      QString dir_path =
          QFileDialog::getExistingDirectory(_widget, "Select export directory", last_dir);
      if (dir_path.isEmpty())
      {
        return;
      }

      QString prefix = getPathPrefix();
      if (prefix.isEmpty())
      {
        prefix = "export";
      }

      settings.setValue("Export.lastDirectory", dir_path);
      emit exportMultipleFiles(is_csv, QDir(dir_path), prefix);
    }
  });
}

ToolBoxUI::~ToolBoxUI()
{
  delete ui;
  delete _widget;
}

QWidget* ToolBoxUI::widget() const
{
  return _widget;
}

double ToolBoxUI::getStartTime() const
{
  return ui->startTime->value();
}

double ToolBoxUI::getEndTime() const
{
  return ui->endTime->value();
}

double ToolBoxUI::getRelativeTime() const
{
  return _time_offset;
}

PJ::Range ToolBoxUI::getAbsoluteTimeRange() const
{
  PJ::Range range;
  range.min = getStartTime();
  range.max = getEndTime();

  if (isRelativeTime())
  {
    range.min += _time_offset;
    range.max += _time_offset;
  }
  return range;
}

std::vector<std::string> ToolBoxUI::getSelectedTopics() const
{
  // Collect selected topic names from the UI table.
  std::vector<std::string> selected_topics;
  for (int row = 0; row < ui->tableWidget->rowCount(); row++)
  {
    auto* item = ui->tableWidget->item(row, 0);
    if (!item)
    {
      continue;
    }

    const std::string name = item->text().trimmed().toStdString();
    if (!name.empty())
    {
      selected_topics.push_back(name);
    }
  }
  return selected_topics;
}

QString ToolBoxUI::getPathPrefix() const
{
  return ui->lineEditPrefix->text().trimmed();
}

bool ToolBoxUI::isRelativeTime() const
{
  return ui->comboTime->currentIndex() == 0;
}

void ToolBoxUI::clearTable(bool clearAll)
{
  auto* table = ui->tableWidget;

  if (clearAll)
  {
    table->setRowCount(0);
    return;
  }

  QModelIndexList rows = table->selectionModel()->selectedRows();
  if (rows.isEmpty())
  {
    return;
  }

  std::sort(rows.begin(), rows.end(),
            [](const QModelIndex& lhs, const QModelIndex& rhs) { return lhs.row() > rhs.row(); });

  for (const auto& idx : rows)
  {
    table->removeRow(idx.row());
  }
}

bool ToolBoxUI::eventFilter(QObject* obj, QEvent* ev)
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

    for (int row = 0; row < ui->tableWidget->rowCount(); row++)
    {
      auto* item = ui->tableWidget->item(row, 0);
      if (item)
      {
        const QString text = item->text().trimmed();
        if (!text.isEmpty())
        {
          existing.insert(text);
        }
      }
    }

    for (const QString& raw : _dragging_curves)
    {
      const QString topic = raw.trimmed();
      if (topic.isEmpty())
      {
        continue;
      }

      if (existing.contains(topic))
      {
        continue;
      }

      existing.insert(topic);

      const int row = ui->tableWidget->rowCount();
      ui->tableWidget->insertRow(row);
      ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
    }

    _dragging_curves.clear();
    updateTimeControlsEnabled();
    emit recomputeTime();
    event->acceptProposedAction();
    return true;
  }

  return false;
}

// Disable time and export controls when no topics are selected.
void ToolBoxUI::updateTimeControlsEnabled()
{
  const bool has_data = ui->tableWidget->rowCount() > 0;
  ui->startTime->setEnabled(has_data);
  ui->endTime->setEnabled(has_data);
  ui->rangeSlider->setEnabled(has_data);
  ui->saveButton->setEnabled(has_data);
}

// Update UI time range in relative or absolute mode.
void ToolBoxUI::setTimeRange(double tmin, double tmax)
{
  // Update slider/spinboxes with either relative time (starting at 0) or absolute time.
  const bool relative = ui->comboTime->currentIndex() == 0;
  const int decimals = 3;

  QSignalBlocker b0(*ui->rangeSlider);
  QSignalBlocker b1(*ui->startTime);
  QSignalBlocker b2(*ui->endTime);

  if (relative)
  {
    _time_offset = tmin;
    double duration = tmax - tmin;
    if (duration < 0.0)
    {
      duration = 0.0;
    }

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
    _time_offset = 0.0;

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

void ToolBoxUI::setTopics(const std::vector<std::string>& topics)
{
  QSet<QString> existing;
  existing.reserve(ui->tableWidget->rowCount());

  for (int row = 0; row < ui->tableWidget->rowCount(); row++)
  {
    auto* item = ui->tableWidget->item(row, 0);
    if (item)
    {
      const QString t = item->text().trimmed();
      if (!t.isEmpty())
      {
        existing.insert(t);
      }
    }
  }

  for (const auto& s : topics)
  {
    const QString topic = QString::fromStdString(s).trimmed();
    if (topic.isEmpty() || existing.contains(topic))
    {
      continue;
    }

    existing.insert(topic);

    const int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
  }

  updateTimeControlsEnabled();
  emit recomputeTime();
}
