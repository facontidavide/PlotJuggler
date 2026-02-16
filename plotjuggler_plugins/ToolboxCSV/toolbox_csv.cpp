#include "toolbox_csv.h"
#include "ui_toolbox_csv.h"

#include <QEvent>
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

  // Solved UI style problems(Black blocks)
  ui->toolButton->setIcon(_widget->style()->standardIcon(QStyle::SP_DirOpenIcon));
  ui->toolButton->setAutoRaise(true);
  ui->toolButton->setStyleSheet("QToolButton { background: transparent; }");

  auto* corner = ui->tableWidget->findChild<QAbstractButton*>();
  if (corner)
  {
    corner->setToolTip("Click to select all topics");
  }

  ui->tableWidget->setStyleSheet(
      "QTableCornerButton::section:hover {"
      "  background-color: palette(light);"
      "}"
      );

  // Drag/Drop event
  ui->tableWidget->installEventFilter(this);
  ui->tableWidget->viewport()->installEventFilter(this);

  ui->plotBox->setChecked(true);
  ui->framePlotPreview->setVisible(true);

  // Visible/Invisible plot data
  connect(ui->plotBox, &QCheckBox::toggled, this, [this](bool on) {
    ui->framePlotPreview->setVisible(on);
  });

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

  auto preview_layout = new QHBoxLayout(ui->framePlotPreview);
  preview_layout->setMargin(6);
  preview_layout->addWidget(_plot_widget);
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxCSV::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::WidgetType::FIXED };
}

bool ToolboxCSV::onShowWidget()
{
  return true;
}

bool ToolboxCSV::eventFilter(QObject* obj, QEvent* ev) {
  if (ev->type() == QEvent::DragEnter) {
    auto* event = static_cast<QDragEnterEvent*>(ev);
    const QMimeData* mimeData = event->mimeData();
    const QStringList mimeFormats = mimeData->formats();

    for (const QString& format : mimeFormats) {
      if (format != "curveslist/add_curve") {
        continue;
      }

      QByteArray encoded = mimeData->data(format);
      QDataStream stream(&encoded, QIODevice::ReadOnly);

      QStringList curves;
      while (!stream.atEnd()) {
        QString curve_name;
        stream >> curve_name;
        if (!curve_name.isEmpty()) {
          curves.push_back(curve_name);
        }
      }

      if (curves.isEmpty()) {
        return false;
      }

      _dragging_curves = curves;

      if (obj == ui->tableWidget || obj == ui->tableWidget->viewport()) {
        event->acceptProposedAction();
        return true;
      }
    }

    return false;
  } else if (ev->type() == QEvent::DragMove) {
    auto* event = static_cast<QDragMoveEvent*>(ev);
    if (obj == ui->tableWidget || obj == ui->tableWidget->viewport()) {
      if (event->mimeData() && event->mimeData()->hasFormat("curveslist/add_curve")) {
        event->acceptProposedAction();
        return true;
      }
    }
    return false;
  } else if (ev->type() == QEvent::Drop) {
    auto* event = static_cast<QDropEvent*>(ev);

    if (obj != ui->tableWidget && obj != ui->tableWidget->viewport()) {
      return false;
    }

    for (const QString& raw : _dragging_curves) {
      QString topic = raw.trimmed();
      if (topic.isEmpty()) {
        continue;
      }

      bool exists = false;
      for (int r = 0; r < ui->tableWidget->rowCount(); r++) {
        QTableWidgetItem* item = ui->tableWidget->item(r, 0);
        if (item && item->text() == topic) {
          exists = true;
          break;
        }
      }

      if (exists) {
        continue;
      }

      int row = ui->tableWidget->rowCount();
      ui->tableWidget->insertRow(row);
      ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
    }

    _dragging_curves.clear();
    event->acceptProposedAction();
    return true;
  }

  return false;
}

void ToolboxCSV::onClosed()
{
  emit this->closed();
}

