#include "markers_panel.h"
#include "ui_markers_panel.h"

#include "PlotJuggler/svg_util.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QSettings>
#include <QSet>
#include <QSignalBlocker>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <algorithm>
#include <limits>

namespace
{
constexpr int COLUMN_ON = 0;
constexpr int COLUMN_NAME = 1;
constexpr int COLUMN_TYPE = 2;
constexpr int COLUMN_START = 3;
constexpr int COLUMN_END = 4;
constexpr int COLUMN_LABEL = 5;
constexpr int COLUMN_TAGS = 6;
constexpr int COLUMN_NOTES = 7;
constexpr int COLUMN_STATE = 8;

constexpr int ROLE_LAYER_INDEX = Qt::UserRole;
constexpr int ROLE_MARKER_ROW = Qt::UserRole + 1;

QIcon colorSwatchIcon(const QColor& color)
{
  QPixmap pixmap(12, 12);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(QPen(Qt::black, 1));
  painter.setBrush(color);
  painter.drawRoundedRect(QRectF(1, 1, 10, 10), 2, 2);
  return QIcon(pixmap);
}

QIcon editPenIcon()
{
  QSettings settings;
  const QString theme = settings.value("StyleSheet::theme", "light").toString();
  return QIcon(LoadSvg(":/resources/svg/pencil-edit.svg", theme));
}
}  // namespace

MarkersPanel::MarkersPanel(MarkerManager* manager, QWidget* parent)
  : QWidget(parent), _manager(manager)
  , ui(new Ui::MarkersPanel)
{
  ui->setupUi(this);

  ui->treeWidgetMarkup->setRootIsDecorated(true);
  ui->treeWidgetMarkup->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->treeWidgetMarkup->setUniformRowHeights(true);
  ui->treeWidgetMarkup->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeWidgetMarkup->header()->setStretchLastSection(false);
  ui->treeWidgetMarkup->header()->setSectionResizeMode(COLUMN_NOTES, QHeaderView::Stretch);
  ui->treeWidgetMarkup->header()->setSectionResizeMode(COLUMN_LABEL, QHeaderView::Stretch);

  connect(ui->buttonNewLayer, &QPushButton::clicked, this, &MarkersPanel::onNewLayer);
  connect(ui->buttonLoadLayer, &QPushButton::clicked, this, &MarkersPanel::onLoadLayer);
  connect(ui->buttonSaveLayer, &QPushButton::clicked, this, &MarkersPanel::onSaveLayer);
  connect(ui->buttonSaveLayerAs, &QPushButton::clicked, this, &MarkersPanel::onSaveLayerAs);
  connect(ui->buttonDuplicateLayer, &QPushButton::clicked, this,
          &MarkersPanel::onDuplicateLayer);
  connect(ui->buttonRenameLayer, &QPushButton::clicked, this, &MarkersPanel::onRenameLayer);
  connect(ui->buttonRemoveLayer, &QPushButton::clicked, this, &MarkersPanel::onRemoveLayer);
  connect(ui->checkBoxAutoloadCompanionMarkup, &QCheckBox::toggled, this,
          &MarkersPanel::autoloadCompanionMarkupChanged);
  connect(ui->buttonMarkRange, &QPushButton::clicked, this, &MarkersPanel::onAddRegion);
  connect(ui->buttonDeleteItem, &QPushButton::clicked, this, &MarkersPanel::onRemoveItem);
  connect(ui->buttonJumpTo, &QPushButton::clicked, this, &MarkersPanel::onJumpToItem);

  connect(ui->treeWidgetMarkup, &QTreeWidget::currentItemChanged, this,
          &MarkersPanel::onTreeCurrentItemChanged);
  connect(ui->treeWidgetMarkup, &QTreeWidget::itemChanged, this, &MarkersPanel::onTreeItemChanged);
  connect(ui->treeWidgetMarkup, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem* item, int column) {
            if (!item)
            {
              return;
            }
            if (layerIndexFromItem(item) >= 0 && markerRowFromItem(item) < 0)
            {
              if (const auto* layer = _manager->activeLayer())
              {
                onEditableToggled(!layer->editable);
              }
            }
            else if (markerRowFromItem(item) >= 0 && column != COLUMN_ON)
            {
              ui->treeWidgetMarkup->editItem(item, column);
            }
          });

  connect(_manager, &MarkerManager::layersChanged, this, [this]() { scheduleTreeRefresh(); });
  connect(_manager, &MarkerManager::itemsChanged, this, [this]() { scheduleTreeRefresh(); });
  connect(_manager, &MarkerManager::activeLayerChanged, this, [this](int index) {
    Q_UNUSED(index);
    scheduleTreeRefresh();
  });

  populateTree();
  updateButtons();
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

void MarkersPanel::setStreamingActive(bool active)
{
  _streaming_active = active;
  updateButtons();
}

void MarkersPanel::setSessionDataFiles(const QStringList& file_paths)
{
  _session_data_files = file_paths;
  updateButtons();
}

void MarkersPanel::setCurrentAxisId(const QString& axis_id)
{
  if (_current_axis_id == axis_id)
  {
    return;
  }
  _current_axis_id = axis_id;
  scheduleTreeRefresh();
}

void MarkersPanel::setAutoloadCompanionMarkup(bool enabled)
{
  ui->checkBoxAutoloadCompanionMarkup->setChecked(enabled);
}

bool MarkersPanel::autoloadCompanionMarkup() const
{
  return ui->checkBoxAutoloadCompanionMarkup->isChecked();
}

bool MarkersPanel::hasSelectedMarker() const
{
  return selectedMarkerRow() >= 0 && isCurrentSelectionCompatible();
}

MarkerManager::MarkerItem MarkersPanel::selectedMarker() const
{
  auto* item = currentTreeItem();
  const int row = markerRowFromItem(item);
  const int layer_index = layerIndexFromItem(item);
  if (row < 0 || layer_index < 0)
  {
    return {};
  }
  const auto& layers = _manager->layers();
  if (layer_index >= layers.size())
  {
    return {};
  }
  const auto* layer = &layers[layer_index];
  if (!layer || row >= layer->items.size())
  {
    return {};
  }
  return layer->items[row];
}

int MarkersPanel::selectedMarkerRow() const
{
  return markerRowFromItem(currentTreeItem());
}

bool MarkersPanel::isActiveLayerEditable() const
{
  const auto* layer = _manager->activeLayer();
  return layer && layer->editable && isLayerCompatible(*layer);
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
  scheduleTreeRefresh();
}

void MarkersPanel::refreshItems()
{
  scheduleTreeRefresh();
}

void MarkersPanel::onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
  Q_UNUSED(previous);
  if (_updating_ui)
  {
    return;
  }
  if (!current)
  {
    updateButtons();
    emit selectedMarkerChanged(false);
    return;
  }
  const int layer_index = layerIndexFromItem(current);
  if (layer_index >= 0)
  {
    _manager->setActiveLayerIndex(layer_index);
  }
  updateButtons();
  emit selectedMarkerChanged(hasSelectedMarker());
}

void MarkersPanel::onTreeItemChanged(QTreeWidgetItem* item, int column)
{
  if (_updating_ui || !item)
  {
    return;
  }

  const int layer_index = layerIndexFromItem(item);
  const int marker_row = markerRowFromItem(item);

  if (layer_index < 0)
  {
    return;
  }

  if (marker_row < 0)
  {
    if (column == COLUMN_ON)
    {
      const auto& layers = _manager->layers();
      if (layer_index < layers.size() && !isLayerCompatible(layers[layer_index]))
      {
        const QSignalBlocker tree_blocker(ui->treeWidgetMarkup);
        item->setCheckState(COLUMN_ON, Qt::Unchecked);
        return;
      }
      _manager->setLayerVisible(layer_index, item->checkState(COLUMN_ON) == Qt::Checked);
    }
    return;
  }

  if (_manager->activeLayerIndex() != layer_index)
  {
    _manager->setActiveLayerIndex(layer_index);
  }
  if (column == COLUMN_LABEL)
  {
    const QSignalBlocker tree_blocker(ui->treeWidgetMarkup);
    item->setText(COLUMN_NAME, item->text(COLUMN_LABEL).isEmpty() ? "<marker>" : item->text(COLUMN_LABEL));
  }
  else if (column == COLUMN_NAME)
  {
    const QSignalBlocker tree_blocker(ui->treeWidgetMarkup);
    item->setText(COLUMN_LABEL, item->text(COLUMN_NAME) == "<marker>" ? QString() : item->text(COLUMN_NAME));
  }
  _manager->updateActiveLayerItem(marker_row, markerItemFromTree(item));
}

void MarkersPanel::onNewLayer()
{
  const int index = _manager->createLayer(suggestUniqueLayerName());
  if (index >= 0 && !_current_axis_id.isEmpty())
  {
    _manager->setLayerAxisId(index, _current_axis_id, true);
  }
}

void MarkersPanel::onLoadLayer()
{
  const QString file_path = QFileDialog::getOpenFileName(
      this, tr("Load Marker Layer"), defaultMarkupDirectory(),
      tr("Marker Files (*.markup*.json *.json);;All Files (*)"));
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
  if (!layer->axis_explicit && !_current_axis_id.isEmpty())
  {
    _manager->setLayerAxisId(index, _current_axis_id, true);
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
  if (const auto* layer = _manager->activeLayer();
      layer && !layer->axis_explicit && !_current_axis_id.isEmpty())
  {
    _manager->setLayerAxisId(index, _current_axis_id, true);
  }
  const QString file_path = QFileDialog::getSaveFileName(
      this, tr("Save Marker Layer As"), suggestUniqueMarkupFilePath(),
      tr("Marker Files (*.markup*.json);;JSON Files (*.json);;All Files (*)"));
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
  const QString suggested_name = suggestDuplicateLayerName(layer->name);
  const QString name = QInputDialog::getText(this, tr("Duplicate Marker Layer"),
                                             tr("New layer name:"), QLineEdit::Normal,
                                             suggested_name, &ok);
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

void MarkersPanel::onAddRegion()
{
  MarkerManager::MarkerItem item;
  item.type = MarkerManager::MarkerType::Region;
  const double view_width = std::abs(_view_end_time - _view_start_time);
  const double width =
      (view_width > std::numeric_limits<double>::epsilon()) ? std::max(view_width * 0.1, 1e-9) :
                                                              1.0;

  if (_streaming_active)
  {
    item.start_time = _current_time;
    item.end_time = _current_time + width;
    item.label = QString("Range @ %1").arg(_current_time, 0, 'f', 3);
  }
  else
  {
    const double center =
        (view_width > std::numeric_limits<double>::epsilon()) ? 0.5 * (_view_start_time + _view_end_time) :
                                                                _current_time;
    item.start_time = center - 0.5 * width;
    item.end_time = center + 0.5 * width;
    item.label = QString("Range @ %1").arg(center, 0, 'f', 3);
  }

  if (const auto* layer = _manager->activeLayer())
  {
    item.color = layer->color;
  }
  _manager->addItemToActiveLayer(item);
}

void MarkersPanel::onRemoveItem()
{
  const int row = selectedMarkerRow();
  if (row < 0)
  {
    return;
  }
  _manager->removeActiveLayerItem(row);
}

void MarkersPanel::onJumpToItem()
{
  if (hasSelectedMarker())
  {
    emit jumpToSelectedMarkerRequested();
  }
}

void MarkersPanel::scheduleTreeRefresh()
{
  if (_tree_refresh_pending)
  {
    return;
  }
  _tree_refresh_pending = true;
  QTimer::singleShot(0, this, [this]() {
    _tree_refresh_pending = false;
    populateTree();
    updateButtons();
  });
}

void MarkersPanel::updateButtons()
{
  _updating_ui = true;

  const auto* layer = _manager->activeLayer();
  const bool has_layer = (layer != nullptr);
  const bool has_dataset = !_session_data_files.isEmpty();
  const bool compatible = has_layer && isLayerCompatible(*layer);
  const bool editable = has_layer && layer->editable && compatible;

  ui->buttonNewLayer->setEnabled(has_dataset);
  ui->buttonLoadLayer->setEnabled(has_dataset);
  ui->buttonSaveLayer->setEnabled(has_layer);
  ui->buttonSaveLayerAs->setEnabled(has_layer);
  ui->buttonDuplicateLayer->setEnabled(has_layer);
  ui->buttonRemoveLayer->setEnabled(has_layer);
  ui->buttonRenameLayer->setEnabled(has_layer);
  ui->buttonMarkRange->setEnabled(editable);
  ui->buttonDeleteItem->setEnabled(editable && hasSelectedMarker());
  ui->buttonJumpTo->setEnabled(hasSelectedMarker());
  ui->checkBoxAutoloadCompanionMarkup->setEnabled(has_dataset);
  ui->treeWidgetMarkup->setEnabled(has_layer || has_dataset);

  _updating_ui = false;
}

bool MarkersPanel::isLayerCompatible(const MarkerManager::MarkerLayer& layer) const
{
  if (!layer.axis_explicit || layer.axis_id.isEmpty() || _current_axis_id.isEmpty())
  {
    return true;
  }
  return layer.axis_id == _current_axis_id;
}

bool MarkersPanel::isCurrentSelectionCompatible() const
{
  const int layer_index = layerIndexFromItem(currentTreeItem());
  const auto& layers = _manager->layers();
  if (layer_index < 0 || layer_index >= layers.size())
  {
    return true;
  }
  return isLayerCompatible(layers[layer_index]);
}

void MarkersPanel::populateTree()
{
  _updating_ui = true;
  auto* current = currentTreeItem();
  const int current_layer = layerIndexFromItem(current);
  const int current_marker = markerRowFromItem(current);

  QSignalBlocker tree_blocker(ui->treeWidgetMarkup);
  ui->treeWidgetMarkup->clear();

  const auto& layers = _manager->layers();
  for (int i = 0; i < layers.size(); ++i)
  {
    addLayerItem(i, layers[i]);
  }

  QTreeWidgetItem* restore_item = nullptr;
  if (current_layer >= 0 && current_layer < ui->treeWidgetMarkup->topLevelItemCount())
  {
    restore_item = ui->treeWidgetMarkup->topLevelItem(current_layer);
    if (restore_item && current_marker >= 0 && current_marker < restore_item->childCount())
    {
      restore_item = restore_item->child(current_marker);
    }
  }
  if (!restore_item && _manager->activeLayerIndex() >= 0 &&
      _manager->activeLayerIndex() < ui->treeWidgetMarkup->topLevelItemCount())
  {
    restore_item = ui->treeWidgetMarkup->topLevelItem(_manager->activeLayerIndex());
  }
  if (restore_item)
  {
    ui->treeWidgetMarkup->setCurrentItem(restore_item);
  }

  _updating_ui = false;
  emit selectedMarkerChanged(hasSelectedMarker());
}

MarkerManager::MarkerItem MarkersPanel::markerItemFromTree(QTreeWidgetItem* item) const
{
  MarkerManager::MarkerItem marker;
  const int layer_index = layerIndexFromItem(item);
  const int marker_row = markerRowFromItem(item);
  const auto& layers = _manager->layers();
  if (layer_index >= 0 && layer_index < layers.size() && marker_row >= 0 &&
      marker_row < layers[layer_index].items.size())
  {
    marker = layers[layer_index].items[marker_row];
  }

  marker.enabled = (item->checkState(COLUMN_ON) == Qt::Checked);
  marker.type = (item->text(COLUMN_TYPE).compare("Region", Qt::CaseInsensitive) == 0) ?
                    MarkerManager::MarkerType::Region :
                    MarkerManager::MarkerType::Point;
  marker.start_time = item->text(COLUMN_START).toDouble();
  marker.end_time = item->text(COLUMN_END).toDouble();
  marker.label = item->text(COLUMN_LABEL);
  marker.tags = item->text(COLUMN_TAGS);
  marker.notes = item->text(COLUMN_NOTES);
  return marker;
}

void MarkersPanel::addLayerItem(int layer_index, const MarkerManager::MarkerLayer& layer)
{
  auto* layer_item = new QTreeWidgetItem(ui->treeWidgetMarkup);
  layer_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
  layer_item->setFlags(layer_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable |
                       Qt::ItemIsEnabled);
  const bool compatible = isLayerCompatible(layer);
  layer_item->setCheckState(COLUMN_ON, (layer.visible && compatible) ? Qt::Checked : Qt::Unchecked);

  QString layer_name = layer.name;
  if (layer.dirty)
  {
    layer_name += " *";
  }
  layer_item->setText(COLUMN_NAME, layer_name);
  layer_item->setText(COLUMN_TYPE, "Layer");
  const bool is_active_edit_layer = (layer_index == _manager->activeLayerIndex() && layer.editable);
  layer_item->setText(COLUMN_STATE, !compatible           ? "Axis mismatch" :
                                    is_active_edit_layer ? "Editing" :
                                    (!layer.editable)    ? "Read only" :
                                                           QString());
  layer_item->setIcon(COLUMN_NAME, colorSwatchIcon(layer.color));
  if (is_active_edit_layer && compatible)
  {
    layer_item->setIcon(COLUMN_STATE, editPenIcon());
  }
  layer_item->setToolTip(COLUMN_NAME, layer.file_path.isEmpty() ? layer.name : layer.file_path);
  layer_item->setToolTip(COLUMN_STATE,
                         !compatible ? QString("Markup axis '%1' does not match current axis '%2'")
                                           .arg(layer.axis_id, _current_axis_id) :
                         is_active_edit_layer ? "Active editable markup set" :
                         (!layer.editable)    ? "Read-only markup set" :
                                                "Double-click to make this the editable markup set");
  layer_item->setExpanded(true);

  auto layer_font = layer_item->font(COLUMN_NAME);
  layer_font.setBold(true);
  layer_item->setFont(COLUMN_NAME, layer_font);

  if (!compatible)
  {
    layer_item->setForeground(COLUMN_STATE, QBrush(QColor("#c2410c")));
  }
  else if (is_active_edit_layer)
  {
    layer_item->setForeground(COLUMN_STATE, QBrush(palette().highlight()));
  }

  for (int row = 0; row < layer.items.size(); ++row)
  {
    const auto& marker = layer.items[row];
    auto* marker_item = new QTreeWidgetItem(layer_item);
    marker_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
    marker_item->setData(COLUMN_NAME, ROLE_MARKER_ROW, row);
    marker_item->setFlags(marker_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable |
                          Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    marker_item->setCheckState(COLUMN_ON, marker.enabled ? Qt::Checked : Qt::Unchecked);
    marker_item->setText(COLUMN_NAME, marker.label.isEmpty() ? "<marker>" : marker.label);
    marker_item->setText(COLUMN_TYPE,
                         marker.type == MarkerManager::MarkerType::Point ? "Point" : "Region");
    marker_item->setText(COLUMN_START, QString::number(marker.start_time, 'f', 6));
    marker_item->setText(COLUMN_END, QString::number(marker.end_time, 'f', 6));
    marker_item->setText(COLUMN_LABEL, marker.label);
    marker_item->setText(COLUMN_TAGS, marker.tags);
    marker_item->setText(COLUMN_NOTES, marker.notes);
    marker_item->setToolTip(COLUMN_NAME, marker.notes);
  }
}

QTreeWidgetItem* MarkersPanel::currentTreeItem() const
{
  return ui->treeWidgetMarkup->currentItem();
}

int MarkersPanel::layerIndexFromItem(const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return -1;
  }
  return item->data(COLUMN_NAME, ROLE_LAYER_INDEX).toInt();
}

int MarkersPanel::markerRowFromItem(const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return -1;
  }
  if (!item->parent())
  {
    return -1;
  }
  return item->data(COLUMN_NAME, ROLE_MARKER_ROW).toInt();
}

QString MarkersPanel::defaultMarkupDirectory() const
{
  if (!_session_data_files.isEmpty())
  {
    return QFileInfo(_session_data_files.front()).absolutePath();
  }
  const auto* layer = _manager->activeLayer();
  if (layer && !layer->file_path.isEmpty())
  {
    return QFileInfo(layer->file_path).absolutePath();
  }
  return QString();
}

QString MarkersPanel::defaultMarkupStem() const
{
  if (!_session_data_files.isEmpty())
  {
    return QFileInfo(_session_data_files.front()).completeBaseName() + ".markup";
  }
  const auto* layer = _manager->activeLayer();
  if (layer && !layer->name.trimmed().isEmpty())
  {
    return layer->name.trimmed();
  }
  return "markers";
}

QString MarkersPanel::suggestUniqueLayerName() const
{
  const QString base = defaultMarkupStem();
  QSet<QString> used_names;
  for (const auto& layer : _manager->layers())
  {
    used_names.insert(layer.name);
  }

  QString candidate = base;
  int suffix = 2;
  while (used_names.contains(candidate))
  {
    candidate = QString("%1.%2").arg(base).arg(suffix++);
  }
  return candidate;
}

QString MarkersPanel::suggestDuplicateLayerName(const QString& source_name) const
{
  QString base = source_name.trimmed();
  if (base.isEmpty())
  {
    base = defaultMarkupStem();
  }

  QSet<QString> used_names;
  for (const auto& layer : _manager->layers())
  {
    used_names.insert(layer.name);
  }

  QString candidate = base + " copy";
  int suffix = 2;
  while (used_names.contains(candidate))
  {
    candidate = QString("%1.%2").arg(base).arg(suffix++);
  }
  return candidate;
}

QString MarkersPanel::sanitizeMarkupVariant(QString text) const
{
  text = text.trimmed();
  text.replace(QRegularExpression("\\s+"), "_");
  text.replace(QRegularExpression("[^A-Za-z0-9._-]+"), "_");
  text.replace(QRegularExpression("_+"), "_");
  text.replace(QRegularExpression("\\.+"), ".");
  text.remove(QRegularExpression("^[._-]+"));
  text.remove(QRegularExpression("[._-]+$"));
  return text;
}

QString MarkersPanel::autoloadSafeMarkupBaseName(const QString& layer_name) const
{
  const QString session_stem =
      !_session_data_files.isEmpty() ? QFileInfo(_session_data_files.front()).completeBaseName() :
                                       QString();
  if (session_stem.isEmpty())
  {
    return sanitizeMarkupVariant(layer_name);
  }

  const QString markup_prefix = session_stem + ".markup";
  QString variant = layer_name.trimmed();

  if (variant.compare(markup_prefix, Qt::CaseInsensitive) == 0)
  {
    return markup_prefix;
  }

  if (variant.startsWith(markup_prefix, Qt::CaseInsensitive))
  {
    variant = variant.mid(markup_prefix.size());
  }

  variant.replace(QRegularExpression("^[\\s._()-]+"), "");
  variant = sanitizeMarkupVariant(variant);

  if (variant.isEmpty())
  {
    return markup_prefix;
  }
  return markup_prefix + "." + variant;
}

QString MarkersPanel::suggestUniqueMarkupFilePath() const
{
  const QDir dir(defaultMarkupDirectory());
  const QString layer_name =
      _manager->activeLayer() && !_manager->activeLayer()->name.trimmed().isEmpty() ?
          _manager->activeLayer()->name.trimmed() :
          suggestUniqueLayerName();
  const QString base = !_session_data_files.isEmpty() ? autoloadSafeMarkupBaseName(layer_name) :
                                                        sanitizeMarkupVariant(layer_name);

  QString candidate = base;
  int suffix = 2;
  while (dir.exists(candidate + ".json"))
  {
    candidate = QString("%1.%2").arg(base).arg(suffix++);
  }

  return dir.filePath(candidate + ".json");
}
