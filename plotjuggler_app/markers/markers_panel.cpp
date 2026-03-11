#include "markers_panel.h"
#include "ui_markers_panel.h"

#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QTableWidgetItem>
#include <algorithm>
#include <limits>

namespace
{
constexpr int COLUMN_ENABLED = 0;
constexpr int COLUMN_TYPE = 1;
constexpr int COLUMN_START = 2;
constexpr int COLUMN_END = 3;
constexpr int COLUMN_LABEL = 4;
constexpr int COLUMN_TAGS = 5;
constexpr int COLUMN_NOTES = 6;
}  // namespace

MarkersPanel::MarkersPanel(MarkerManager* manager, QWidget* parent)
  : QWidget(parent), _manager(manager)
  , ui(new Ui::MarkersPanel)
{
  ui->setupUi(this);

  ui->tableWidgetItems->setColumnCount(7);
  ui->tableWidgetItems->setHorizontalHeaderLabels(
      { "On", "Type", "Start", "End", "Label", "Tags", "Notes" });
  ui->tableWidgetItems->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  ui->tableWidgetItems->verticalHeader()->setVisible(false);
  ui->tableWidgetItems->setSelectionBehavior(QAbstractItemView::SelectRows);

  connect(ui->buttonNewLayer, &QPushButton::clicked, this, &MarkersPanel::onNewLayer);
  connect(ui->buttonLoadLayer, &QPushButton::clicked, this, &MarkersPanel::onLoadLayer);
  connect(ui->buttonSaveLayer, &QPushButton::clicked, this, &MarkersPanel::onSaveLayer);
  connect(ui->buttonSaveLayerAs, &QPushButton::clicked, this, &MarkersPanel::onSaveLayerAs);
  connect(ui->buttonDuplicateLayer, &QPushButton::clicked, this,
          &MarkersPanel::onDuplicateLayer);
  connect(ui->buttonRenameLayer, &QPushButton::clicked, this, &MarkersPanel::onRenameLayer);
  connect(ui->buttonRemoveLayer, &QPushButton::clicked, this, &MarkersPanel::onRemoveLayer);
  connect(ui->checkBoxLayerEditable, &QCheckBox::toggled, this,
          &MarkersPanel::onEditableToggled);
  connect(ui->buttonMarkNow, &QPushButton::clicked, this, &MarkersPanel::onAddPoint);
  connect(ui->buttonMarkRange, &QPushButton::clicked, this, &MarkersPanel::onAddRegion);
  connect(ui->buttonDeleteItem, &QPushButton::clicked, this, &MarkersPanel::onRemoveItem);
  connect(ui->buttonJumpTo, &QPushButton::clicked, this, &MarkersPanel::onJumpToItem);

  connect(ui->listWidgetLayers, &QListWidget::itemSelectionChanged, this,
          &MarkersPanel::onLayerSelectionChanged);
  connect(ui->listWidgetLayers, &QListWidget::itemChanged, this, &MarkersPanel::onLayerItemChanged);
  connect(ui->tableWidgetItems, &QTableWidget::cellChanged, this, &MarkersPanel::onTableCellChanged);
  connect(ui->tableWidgetItems->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this]() {
            updateButtons();
            emit selectedMarkerChanged(hasSelectedMarker());
          });

  connect(_manager, &MarkerManager::layersChanged, this, &MarkersPanel::refreshLayers);
  connect(_manager, &MarkerManager::itemsChanged, this, &MarkersPanel::refreshItems);
  connect(_manager, &MarkerManager::activeLayerChanged, this, [this](int index) {
    Q_UNUSED(index);
    refreshLayers();
    refreshItems();
  });

  refreshLayers();
  refreshItems();
}

MarkersPanel::~MarkersPanel()
{
  delete ui;
}

void MarkersPanel::setCurrentTime(double time)
{
  _current_time = time;
}

void MarkersPanel::setCurrentViewRange(double start_time, double end_time)
{
  _view_start_time = std::min(start_time, end_time);
  _view_end_time = std::max(start_time, end_time);
}

bool MarkersPanel::hasSelectedMarker() const
{
  return !ui->tableWidgetItems->selectionModel()->selectedRows().isEmpty();
}

MarkerManager::MarkerItem MarkersPanel::selectedMarker() const
{
  const int row = selectedMarkerRow();
  if (row < 0)
  {
    return {};
  }
  const auto* layer = _manager->activeLayer();
  if (!layer || row >= layer->items.size())
  {
    return {};
  }
  return layer->items[row];
}

int MarkersPanel::selectedMarkerRow() const
{
  const auto selected = ui->tableWidgetItems->selectionModel()->selectedRows();
  return selected.isEmpty() ? -1 : selected.front().row();
}

bool MarkersPanel::isActiveLayerEditable() const
{
  const auto* layer = _manager->activeLayer();
  return layer && layer->editable;
}

void MarkersPanel::updateSelectedMarker(const MarkerManager::MarkerItem& item)
{
  const int row = selectedMarkerRow();
  if (row < 0)
  {
    return;
  }

  _manager->updateActiveLayerItem(row, item);
}

void MarkersPanel::refreshLayers()
{
  populateLayerList();
  updateButtons();
}

void MarkersPanel::refreshItems()
{
  populateItemTable();
  updateButtons();
}

void MarkersPanel::onLayerSelectionChanged()
{
  if (_updating_ui)
  {
    return;
  }
  _manager->setActiveLayerIndex(ui->listWidgetLayers->currentRow());
}

void MarkersPanel::onLayerItemChanged(QListWidgetItem* item)
{
  if (_updating_ui || !item)
  {
    return;
  }
  const int row = ui->listWidgetLayers->row(item);
  const bool visible = (item->checkState() == Qt::Checked);
  _manager->setLayerVisible(row, visible);
}

void MarkersPanel::onTableCellChanged(int row, int column)
{
  Q_UNUSED(column);
  if (_updating_ui)
  {
    return;
  }
  _manager->updateActiveLayerItem(row, markerItemFromRow(row));
}

void MarkersPanel::onNewLayer()
{
  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("New Marker Layer"), tr("Layer name:"),
                                             QLineEdit::Normal, "markers", &ok);
  if (ok && !name.trimmed().isEmpty())
  {
    _manager->createLayer(name.trimmed());
  }
}

void MarkersPanel::onLoadLayer()
{
  const QString file_path = QFileDialog::getOpenFileName(
      this, tr("Load Marker Layer"), QString(), tr("Marker Files (*.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->loadLayer(file_path))
  {
    QMessageBox::warning(this, tr("Load failed"), tr("Failed to load marker layer."));
  }
}

void MarkersPanel::onSaveLayer()
{
  const int index = _manager->activeLayerIndex();
  const auto* layer = _manager->activeLayer();
  if (index < 0 || !layer)
  {
    return;
  }

  if (layer->file_path.isEmpty())
  {
    onSaveLayerAs();
    return;
  }

  if (!_manager->saveLayer(index))
  {
    QMessageBox::warning(this, tr("Save failed"), tr("Failed to save marker layer."));
  }
}

void MarkersPanel::onSaveLayerAs()
{
  const int index = _manager->activeLayerIndex();
  if (index < 0)
  {
    return;
  }
  const QString file_path = QFileDialog::getSaveFileName(
      this, tr("Save Marker Layer As"), QString(), tr("Marker Files (*.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->saveLayerAs(index, file_path))
  {
    QMessageBox::warning(this, tr("Save failed"), tr("Failed to save marker layer."));
  }
}

void MarkersPanel::onRemoveLayer()
{
  _manager->removeLayer(_manager->activeLayerIndex());
}

void MarkersPanel::onDuplicateLayer()
{
  const int index = _manager->activeLayerIndex();
  const auto* layer = _manager->activeLayer();
  if (index < 0 || !layer)
  {
    return;
  }

  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("Duplicate Marker Layer"),
                                             tr("New layer name:"), QLineEdit::Normal,
                                             layer->name + " copy", &ok);
  if (ok && !name.trimmed().isEmpty())
  {
    _manager->duplicateLayer(index, name.trimmed());
  }
}

void MarkersPanel::onRenameLayer()
{
  const int index = _manager->activeLayerIndex();
  const auto* layer = _manager->activeLayer();
  if (index < 0 || !layer)
  {
    return;
  }

  bool ok = false;
  const QString name = QInputDialog::getText(this, tr("Rename Marker Layer"), tr("Layer name:"),
                                             QLineEdit::Normal, layer->name, &ok);
  if (ok && !name.trimmed().isEmpty())
  {
    _manager->setLayerName(index, name.trimmed());
  }
}

void MarkersPanel::onEditableToggled(bool checked)
{
  if (_updating_ui)
  {
    return;
  }
  _manager->setLayerEditable(_manager->activeLayerIndex(), checked);
}

void MarkersPanel::onAddPoint()
{
  MarkerManager::MarkerItem item;
  item.type = MarkerManager::MarkerType::Point;
  item.start_time = _current_time;
  item.end_time = _current_time;
  item.label = QString("Mark @ %1").arg(_current_time, 0, 'f', 3);
  _manager->addItemToActiveLayer(item);
}

void MarkersPanel::onAddRegion()
{
  MarkerManager::MarkerItem item;
  item.type = MarkerManager::MarkerType::Region;
  const double view_width = std::abs(_view_end_time - _view_start_time);
  const double center =
      (view_width > std::numeric_limits<double>::epsilon()) ? 0.5 * (_view_start_time + _view_end_time) :
                                                              _current_time;
  const double width = (view_width > std::numeric_limits<double>::epsilon()) ? std::max(view_width * 0.1, 1e-9) :
                                                                                1.0;
  item.start_time = center - 0.5 * width;
  item.end_time = center + 0.5 * width;
  item.label = QString("Range @ %1").arg(center, 0, 'f', 3);
  _manager->addItemToActiveLayer(item);
}

void MarkersPanel::onRemoveItem()
{
  const auto selected = ui->tableWidgetItems->selectionModel()->selectedRows();
  if (selected.isEmpty())
  {
    return;
  }
  _manager->removeActiveLayerItem(selected.front().row());
}

void MarkersPanel::onJumpToItem()
{
  if (hasSelectedMarker())
  {
    emit jumpToSelectedMarkerRequested();
  }
}

void MarkersPanel::updateButtons()
{
  _updating_ui = true;

  const auto* layer = _manager->activeLayer();
  const bool has_layer = (layer != nullptr);
  const bool editable = has_layer && layer->editable;

  ui->buttonSaveLayer->setEnabled(has_layer);
  ui->buttonSaveLayerAs->setEnabled(has_layer);
  ui->buttonDuplicateLayer->setEnabled(has_layer);
  ui->buttonRemoveLayer->setEnabled(has_layer);
  ui->buttonRenameLayer->setEnabled(has_layer);
  ui->buttonMarkNow->setEnabled(editable);
  ui->buttonMarkRange->setEnabled(editable);
  ui->buttonDeleteItem->setEnabled(
      editable && !ui->tableWidgetItems->selectionModel()->selectedRows().isEmpty());
  ui->buttonJumpTo->setEnabled(hasSelectedMarker());
  ui->tableWidgetItems->setEnabled(has_layer);
  ui->checkBoxLayerEditable->setEnabled(has_layer);
  ui->checkBoxLayerEditable->setChecked(editable);
  ui->lineEditLayerName->setEnabled(has_layer);
  ui->lineEditLayerName->setText(has_layer ? layer->name : QString());

  _updating_ui = false;
}

void MarkersPanel::populateLayerList()
{
  _updating_ui = true;
  ui->listWidgetLayers->clear();

  const auto& layers = _manager->layers();
  for (int i = 0; i < layers.size(); ++i)
  {
    const auto& layer = layers[i];
    QString title = layer.name;
    if (layer.dirty)
    {
      title += " *";
    }
    if (!layer.editable)
    {
      title += " [ro]";
    }

    auto* item = new QListWidgetItem(title);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(layer.visible ? Qt::Checked : Qt::Unchecked);
    item->setData(Qt::UserRole + 1, layer.editable);
    item->setToolTip(layer.file_path);
    ui->listWidgetLayers->addItem(item);
  }

  ui->listWidgetLayers->setCurrentRow(_manager->activeLayerIndex());
  _updating_ui = false;
}

void MarkersPanel::populateItemTable()
{
  _updating_ui = true;
  const int selected_row = selectedMarkerRow();
  QSignalBlocker table_blocker(ui->tableWidgetItems);
  QSignalBlocker selection_blocker(ui->tableWidgetItems->selectionModel());
  ui->tableWidgetItems->setRowCount(0);

  const auto* layer = _manager->activeLayer();
  if (layer)
  {
    ui->tableWidgetItems->setRowCount(layer->items.size());
    for (int row = 0; row < layer->items.size(); ++row)
    {
      setTableRow(row, layer->items[row]);
    }
  }

  if (selected_row >= 0 && selected_row < ui->tableWidgetItems->rowCount())
  {
    ui->tableWidgetItems->selectRow(selected_row);
  }

  _updating_ui = false;
  emit selectedMarkerChanged(hasSelectedMarker());
}

MarkerManager::MarkerItem MarkersPanel::markerItemFromRow(int row) const
{
  MarkerManager::MarkerItem item;
  if (auto* enabled = ui->tableWidgetItems->item(row, COLUMN_ENABLED))
  {
    item.enabled = (enabled->checkState() == Qt::Checked);
  }
  if (auto* type = ui->tableWidgetItems->item(row, COLUMN_TYPE))
  {
    item.type = (type->text().compare("Region", Qt::CaseInsensitive) == 0) ?
                    MarkerManager::MarkerType::Region :
                    MarkerManager::MarkerType::Point;
  }
  if (auto* start = ui->tableWidgetItems->item(row, COLUMN_START))
  {
    item.start_time = start->text().toDouble();
  }
  if (auto* end = ui->tableWidgetItems->item(row, COLUMN_END))
  {
    item.end_time = end->text().toDouble();
  }
  if (auto* label = ui->tableWidgetItems->item(row, COLUMN_LABEL))
  {
    item.label = label->text();
  }
  if (auto* tags = ui->tableWidgetItems->item(row, COLUMN_TAGS))
  {
    item.tags = tags->text();
  }
  if (auto* notes = ui->tableWidgetItems->item(row, COLUMN_NOTES))
  {
    item.notes = notes->text();
  }
  return item;
}

void MarkersPanel::setTableRow(int row, const MarkerManager::MarkerItem& item)
{
  auto* enabled_item = new QTableWidgetItem();
  enabled_item->setFlags(enabled_item->flags() | Qt::ItemIsUserCheckable);
  enabled_item->setCheckState(item.enabled ? Qt::Checked : Qt::Unchecked);
  ui->tableWidgetItems->setItem(row, COLUMN_ENABLED, enabled_item);
  ui->tableWidgetItems->setItem(row, COLUMN_TYPE,
                                new QTableWidgetItem(item.type == MarkerManager::MarkerType::Point ?
                                                         "Point" :
                                                         "Region"));
  ui->tableWidgetItems->setItem(row, COLUMN_START,
                                new QTableWidgetItem(QString::number(item.start_time, 'f', 6)));
  ui->tableWidgetItems->setItem(row, COLUMN_END,
                                new QTableWidgetItem(QString::number(item.end_time, 'f', 6)));
  ui->tableWidgetItems->setItem(row, COLUMN_LABEL, new QTableWidgetItem(item.label));
  ui->tableWidgetItems->setItem(row, COLUMN_TAGS, new QTableWidgetItem(item.tags));
  ui->tableWidgetItems->setItem(row, COLUMN_NOTES, new QTableWidgetItem(item.notes));
}
