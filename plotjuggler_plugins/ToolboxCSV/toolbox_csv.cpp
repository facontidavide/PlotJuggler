#include "toolbox_csv.h"
#include "PlotJuggler/svg_util.h"
#include "ui_toolbox_csv.h"

#include <QEvent>
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

  // Drag/Drop event
  ui->tableWidget->installEventFilter(this);
  ui->tableWidget->viewport()->installEventFilter(this);

  // Cancel/Close
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ToolboxCSV::onClosed);
}

ToolboxCSV::~ToolboxCSV()
{
}

void ToolboxCSV::init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map)
{
  _plot_data = &src_data;
  _transforms = &transform_map;

  _plot_widget = new PJ::PlotWidgetBase(ui->frame);
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxCSV::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::WidgetType::FIXED };
}

bool ToolboxCSV::onShowWidget()
{
  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();

  ui->clearButton->setIcon(LoadSvg(":/resources/svg/clear.svg", theme));
  ui->saveButton->setIcon(LoadSvg(":/resources/svg/save.svg", theme));

  ui->csvButton->setChecked(true);

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

  updateTimeControlsEnabled();

  connect(ui->rangeSlider, &RangeSlider::lowerValueChanged, _widget, [this](int v) {
    QSignalBlocker b(*ui->startTime);
    ui->startTime->setValue(ui->rangeSlider->toReal(v));
  });

  connect(ui->rangeSlider, &RangeSlider::upperValueChanged, _widget, [this](int v) {
    QSignalBlocker b(*ui->endTime);
    ui->endTime->setValue(ui->rangeSlider->toReal(v));
  });

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

  // Searcher folder/file button
  connect(ui->toolButton, &QToolButton::clicked, this, &ToolboxCSV::on_toolButton_clicked);

  // Reset Path line if u changed the format
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

  return true;
}

bool ToolboxCSV::eventFilter(QObject* obj, QEvent* ev)
{
  if (ev->type() == QEvent::DragEnter)
  {
    auto* event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    const QStringList mimeFormats = mimeData->formats();

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

    for (const QString& raw : _dragging_curves)
    {
      QString topic = raw.trimmed();
      if (topic.isEmpty())
      {
        continue;
      }

      bool exists = false;
      for (int r = 0; r < ui->tableWidget->rowCount(); r++)
      {
        QTableWidgetItem* item = ui->tableWidget->item(r, 0);
        if (item && item->text() == topic)
        {
          exists = true;
          break;
        }
      }

      if (exists)
      {
        continue;
      }

      int row = ui->tableWidget->rowCount();
      ui->tableWidget->insertRow(row);
      ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
    }

    _dragging_curves.clear();
    updateTimeControlsEnabled();
    event->acceptProposedAction();
    return true;
  }

  return false;
}

void ToolboxCSV::onClosed()
{
  emit this->closed();
}

void ToolboxCSV::on_toolButton_clicked()
{
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

void ToolboxCSV::updateTimeControlsEnabled()
{
  const bool has_data = ui->tableWidget->rowCount() > 0;
  ui->startTime->setEnabled(has_data);
  ui->endTime->setEnabled(has_data);
  ui->rangeSlider->setEnabled(has_data);
  ui->toolButton->setEnabled(has_data);
}
