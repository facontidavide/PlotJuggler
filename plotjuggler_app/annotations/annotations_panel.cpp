#include "annotations_panel.h"
#include "ui_annotations_panel.h"

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
constexpr int ROLE_ANNOTATION_ROW = Qt::UserRole + 1;

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

QString stripTrailingCopySuffix(QString text)
{
  text = text.trimmed();
  text.remove(QRegularExpression("\\s+copy(?:\\s+\\d+)?$", QRegularExpression::CaseInsensitiveOption));
  return text.trimmed();
}
}  // namespace

AnnotationsPanel::AnnotationsPanel(AnnotationManager* manager, QWidget* parent)
  : QWidget(parent), _manager(manager)
  , ui(new Ui::AnnotationsPanel)
{
  ui->setupUi(this);

  ui->treeWidgetAnnotations->setRootIsDecorated(true);
  ui->treeWidgetAnnotations->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->treeWidgetAnnotations->setUniformRowHeights(true);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeWidgetAnnotations->header()->setStretchLastSection(false);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_NOTES, QHeaderView::Stretch);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_LABEL, QHeaderView::Stretch);

  connect(ui->buttonNewLayer, &QPushButton::clicked, this, &AnnotationsPanel::onNewLayer);
  connect(ui->buttonLoadLayer, &QPushButton::clicked, this, &AnnotationsPanel::onLoadLayer);
  connect(ui->buttonSaveLayer, &QPushButton::clicked, this, &AnnotationsPanel::onSaveLayer);
  connect(ui->buttonSaveLayerAs, &QPushButton::clicked, this, &AnnotationsPanel::onSaveLayerAs);
  connect(ui->buttonDuplicateLayer, &QPushButton::clicked, this,
          &AnnotationsPanel::onDuplicateLayer);
  connect(ui->buttonRenameLayer, &QPushButton::clicked, this, &AnnotationsPanel::onRenameLayer);
  connect(ui->buttonRemoveLayer, &QPushButton::clicked, this, &AnnotationsPanel::onRemoveLayer);
  connect(ui->checkBoxAutoloadCompanionAnnotations, &QCheckBox::toggled, this,
          &AnnotationsPanel::autoloadCompanionAnnotationsChanged);
  connect(ui->buttonMarkRange, &QPushButton::clicked, this, &AnnotationsPanel::onAddRegion);
  connect(ui->buttonDeleteItem, &QPushButton::clicked, this, &AnnotationsPanel::onRemoveItem);
  connect(ui->buttonJumpTo, &QPushButton::clicked, this, &AnnotationsPanel::onJumpToItem);

  connect(ui->treeWidgetAnnotations, &QTreeWidget::currentItemChanged, this,
          &AnnotationsPanel::onTreeCurrentItemChanged);
  connect(ui->treeWidgetAnnotations, &QTreeWidget::itemChanged, this, &AnnotationsPanel::onTreeItemChanged);
  connect(ui->treeWidgetAnnotations, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem* item, int column) {
            if (!item)
            {
              return;
            }
            if (layerIndexFromItem(item) >= 0 && annotationRowFromItem(item) < 0)
            {
              if (const auto* layer = _manager->activeLayer())
              {
                onEditableToggled(!layer->editable);
              }
            }
            else if (annotationRowFromItem(item) >= 0 && column != COLUMN_ON)
            {
              ui->treeWidgetAnnotations->editItem(item, column);
            }
          });

  connect(_manager, &AnnotationManager::layersChanged, this, [this]() { scheduleTreeRefresh(); });
  connect(_manager, &AnnotationManager::itemsChanged, this, [this]() { scheduleTreeRefresh(); });
  connect(_manager, &AnnotationManager::activeLayerChanged, this, [this](int index) {
    Q_UNUSED(index);
    scheduleTreeRefresh();
  });

  populateTree();
  updateButtons();
}

AnnotationsPanel::~AnnotationsPanel()
{
  delete ui;
}

void AnnotationsPanel::setCurrentTime(double time)
{
  _current_time = time;
}

void AnnotationsPanel::setCurrentViewRange(double start_time, double end_time)
{
  _view_start_time = std::min(start_time, end_time);
  _view_end_time = std::max(start_time, end_time);
}

void AnnotationsPanel::setStreamingActive(bool active)
{
  _streaming_active = active;
  updateButtons();
}

void AnnotationsPanel::setSessionDataFiles(const QStringList& file_paths)
{
  _session_data_files = file_paths;
  updateButtons();
}

void AnnotationsPanel::setCurrentAxisId(const QString& axis_id)
{
  if (_current_axis_id == axis_id)
  {
    return;
  }
  _current_axis_id = axis_id;
  scheduleTreeRefresh();
}

void AnnotationsPanel::setAutoloadCompanionAnnotations(bool enabled)
{
  ui->checkBoxAutoloadCompanionAnnotations->setChecked(enabled);
}

bool AnnotationsPanel::autoloadCompanionAnnotations() const
{
  return ui->checkBoxAutoloadCompanionAnnotations->isChecked();
}

bool AnnotationsPanel::hasSelectedAnnotation() const
{
  return selectedAnnotationRow() >= 0 && isCurrentSelectionCompatible();
}

AnnotationManager::AnnotationItem AnnotationsPanel::selectedAnnotation() const
{
  auto* item = currentTreeItem();
  const int row = annotationRowFromItem(item);
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

int AnnotationsPanel::selectedAnnotationRow() const
{
  return annotationRowFromItem(currentTreeItem());
}

bool AnnotationsPanel::isActiveLayerEditable() const
{
  const auto* layer = _manager->activeLayer();
  return layer && layer->editable && isLayerCompatible(*layer);
}

void AnnotationsPanel::updateSelectedAnnotation(const AnnotationManager::AnnotationItem& item)
{
  const int row = selectedAnnotationRow();
  if (row < 0)
  {
    return;
  }

  _manager->updateActiveLayerItem(row, item);
}

void AnnotationsPanel::refreshAnnotationLayers()
{
  scheduleTreeRefresh();
}

void AnnotationsPanel::refreshAnnotationItems()
{
  scheduleTreeRefresh();
}

void AnnotationsPanel::onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
  Q_UNUSED(previous);
  if (_updating_ui)
  {
    return;
  }
  if (!current)
  {
    updateButtons();
    emit selectedAnnotationChanged(false);
    return;
  }
  const int layer_index = layerIndexFromItem(current);
  if (layer_index >= 0)
  {
    _manager->setActiveLayerIndex(layer_index);
  }
  updateButtons();
  emit selectedAnnotationChanged(hasSelectedAnnotation());
}

void AnnotationsPanel::onTreeItemChanged(QTreeWidgetItem* item, int column)
{
  if (_updating_ui || !item)
  {
    return;
  }

  const int layer_index = layerIndexFromItem(item);
  const int annotation_row = annotationRowFromItem(item);

  if (layer_index < 0)
  {
    return;
  }

  if (annotation_row < 0)
  {
    if (column == COLUMN_ON)
    {
      const auto& layers = _manager->layers();
      if (layer_index < layers.size() && !isLayerCompatible(layers[layer_index]))
      {
        const QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
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
    const QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
    item->setText(COLUMN_NAME,
                  item->text(COLUMN_LABEL).isEmpty() ? "<annotation>" : item->text(COLUMN_LABEL));
  }
  else if (column == COLUMN_NAME)
  {
    const QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
    item->setText(COLUMN_LABEL,
                  item->text(COLUMN_NAME) == "<annotation>" ? QString() : item->text(COLUMN_NAME));
  }
  _manager->updateActiveLayerItem(annotation_row, annotationItemFromTree(item));
}

void AnnotationsPanel::onNewLayer()
{
  const int index = _manager->createLayer(suggestUniqueLayerName());
  if (index >= 0 && !_current_axis_id.isEmpty())
  {
    _manager->setLayerAxisId(index, _current_axis_id, true);
  }
}

void AnnotationsPanel::onLoadLayer()
{
  const QString file_path = QFileDialog::getOpenFileName(
      this, tr("Load Marker Layer"), defaultAnnotationDirectory(),
      tr("Annotation Files (*.annotations*.json *.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->loadLayer(file_path))
  {
    QMessageBox::warning(this, tr("Load failed"), tr("Failed to load marker layer."));
  }
}

void AnnotationsPanel::onSaveLayer()
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

void AnnotationsPanel::onSaveLayerAs()
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
      this, tr("Save Marker Layer As"), suggestUniqueAnnotationFilePath(),
      tr("Annotation Files (*.annotations*.json);;JSON Files (*.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->saveLayerAs(index, file_path))
  {
    QMessageBox::warning(this, tr("Save failed"), tr("Failed to save marker layer."));
  }
}

void AnnotationsPanel::onRemoveLayer()
{
  _manager->removeLayer(_manager->activeLayerIndex());
}

void AnnotationsPanel::onDuplicateLayer()
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

void AnnotationsPanel::onRenameLayer()
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

void AnnotationsPanel::onEditableToggled(bool checked)
{
  if (_updating_ui)
  {
    return;
  }
  _manager->setLayerEditable(_manager->activeLayerIndex(), checked);
}

void AnnotationsPanel::onAddRegion()
{
  AnnotationManager::AnnotationItem item;
  item.type = AnnotationManager::AnnotationType::Region;
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

void AnnotationsPanel::onRemoveItem()
{
  const int row = selectedAnnotationRow();
  if (row < 0)
  {
    return;
  }
  _manager->removeActiveLayerItem(row);
}

void AnnotationsPanel::onJumpToItem()
{
  if (hasSelectedAnnotation())
  {
    emit jumpToSelectedAnnotationRequested();
  }
}

void AnnotationsPanel::scheduleTreeRefresh()
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

void AnnotationsPanel::updateButtons()
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
  ui->buttonDeleteItem->setEnabled(editable && hasSelectedAnnotation());
  ui->buttonJumpTo->setEnabled(hasSelectedAnnotation());
  ui->checkBoxAutoloadCompanionAnnotations->setEnabled(has_dataset);
  ui->treeWidgetAnnotations->setEnabled(has_layer || has_dataset);

  _updating_ui = false;
}

bool AnnotationsPanel::isLayerCompatible(const AnnotationManager::AnnotationLayer& layer) const
{
  if (!layer.axis_explicit || layer.axis_id.isEmpty() || _current_axis_id.isEmpty())
  {
    return true;
  }
  return layer.axis_id == _current_axis_id;
}

bool AnnotationsPanel::isCurrentSelectionCompatible() const
{
  const int layer_index = layerIndexFromItem(currentTreeItem());
  const auto& layers = _manager->layers();
  if (layer_index < 0 || layer_index >= layers.size())
  {
    return true;
  }
  return isLayerCompatible(layers[layer_index]);
}

void AnnotationsPanel::populateTree()
{
  _updating_ui = true;
  auto* current = currentTreeItem();
  const int current_layer = layerIndexFromItem(current);
  const int current_annotation = annotationRowFromItem(current);

  QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
  ui->treeWidgetAnnotations->clear();

  const auto& layers = _manager->layers();
  for (int i = 0; i < layers.size(); ++i)
  {
    addLayerItem(i, layers[i]);
  }

  QTreeWidgetItem* restore_item = nullptr;
  if (current_layer >= 0 && current_layer < ui->treeWidgetAnnotations->topLevelItemCount())
  {
    restore_item = ui->treeWidgetAnnotations->topLevelItem(current_layer);
    if (restore_item && current_annotation >= 0 && current_annotation < restore_item->childCount())
    {
      restore_item = restore_item->child(current_annotation);
    }
  }
  if (!restore_item && _manager->activeLayerIndex() >= 0 &&
      _manager->activeLayerIndex() < ui->treeWidgetAnnotations->topLevelItemCount())
  {
    restore_item = ui->treeWidgetAnnotations->topLevelItem(_manager->activeLayerIndex());
  }
  if (restore_item)
  {
    ui->treeWidgetAnnotations->setCurrentItem(restore_item);
  }

  _updating_ui = false;
  emit selectedAnnotationChanged(hasSelectedAnnotation());
}

AnnotationManager::AnnotationItem AnnotationsPanel::annotationItemFromTree(QTreeWidgetItem* item) const
{
  AnnotationManager::AnnotationItem annotation;
  const int layer_index = layerIndexFromItem(item);
  const int annotation_row = annotationRowFromItem(item);
  const auto& layers = _manager->layers();
  if (layer_index >= 0 && layer_index < layers.size() && annotation_row >= 0 &&
      annotation_row < layers[layer_index].items.size())
  {
    annotation = layers[layer_index].items[annotation_row];
  }

  annotation.enabled = (item->checkState(COLUMN_ON) == Qt::Checked);
  annotation.type = (item->text(COLUMN_TYPE).compare("Region", Qt::CaseInsensitive) == 0) ?
                    AnnotationManager::AnnotationType::Region :
                    AnnotationManager::AnnotationType::Point;
  annotation.start_time = item->text(COLUMN_START).toDouble();
  annotation.end_time = item->text(COLUMN_END).toDouble();
  annotation.label = item->text(COLUMN_LABEL);
  annotation.tags = item->text(COLUMN_TAGS);
  annotation.notes = item->text(COLUMN_NOTES);
  return annotation;
}

void AnnotationsPanel::addLayerItem(int layer_index, const AnnotationManager::AnnotationLayer& layer)
{
  auto* layer_item = new QTreeWidgetItem(ui->treeWidgetAnnotations);
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
                         !compatible ? QString("Annotation axis '%1' does not match current axis '%2'")
                                           .arg(layer.axis_id, _current_axis_id) :
                         is_active_edit_layer ? "Active editable annotation set" :
                         (!layer.editable)    ? "Read-only annotation set" :
                                                "Double-click to make this the editable annotation set");
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
    const auto& annotation = layer.items[row];
    auto* annotation_item = new QTreeWidgetItem(layer_item);
    annotation_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
    annotation_item->setData(COLUMN_NAME, ROLE_ANNOTATION_ROW, row);
    annotation_item->setFlags(annotation_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable |
                          Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    annotation_item->setCheckState(COLUMN_ON, annotation.enabled ? Qt::Checked : Qt::Unchecked);
    annotation_item->setText(COLUMN_NAME, annotation.label.isEmpty() ? "<annotation>" : annotation.label);
    annotation_item->setText(COLUMN_TYPE,
                         annotation.type == AnnotationManager::AnnotationType::Point ? "Point" : "Region");
    annotation_item->setText(COLUMN_START, QString::number(annotation.start_time, 'f', 6));
    annotation_item->setText(COLUMN_END, QString::number(annotation.end_time, 'f', 6));
    annotation_item->setText(COLUMN_LABEL, annotation.label);
    annotation_item->setText(COLUMN_TAGS, annotation.tags);
    annotation_item->setText(COLUMN_NOTES, annotation.notes);
    annotation_item->setToolTip(COLUMN_NAME, annotation.notes);
  }
}

QTreeWidgetItem* AnnotationsPanel::currentTreeItem() const
{
  return ui->treeWidgetAnnotations->currentItem();
}

int AnnotationsPanel::layerIndexFromItem(const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return -1;
  }
  return item->data(COLUMN_NAME, ROLE_LAYER_INDEX).toInt();
}

int AnnotationsPanel::annotationRowFromItem(const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return -1;
  }
  if (!item->parent())
  {
    return -1;
  }
  return item->data(COLUMN_NAME, ROLE_ANNOTATION_ROW).toInt();
}

QString AnnotationsPanel::defaultAnnotationDirectory() const
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

QString AnnotationsPanel::defaultAnnotationStem() const
{
  if (!_session_data_files.isEmpty())
  {
    return QFileInfo(_session_data_files.front()).completeBaseName() + ".annotations";
  }
  const auto* layer = _manager->activeLayer();
  if (layer && !layer->name.trimmed().isEmpty())
  {
    return layer->name.trimmed();
  }
  return "markers";
}

QString AnnotationsPanel::suggestUniqueLayerName() const
{
  const QString base = defaultAnnotationStem();
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

QString AnnotationsPanel::suggestDuplicateLayerName(const QString& source_name) const
{
  QString base = stripTrailingCopySuffix(source_name);
  if (base.isEmpty())
  {
    base = defaultAnnotationStem();
  }

  QSet<QString> used_names;
  for (const auto& layer : _manager->layers())
  {
    used_names.insert(layer.name);
  }
  return nextCopyLayerName(base, used_names);
}

QString AnnotationsPanel::sanitizeAnnotationVariant(QString text) const
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

QString AnnotationsPanel::autoloadSafeAnnotationBaseName(const QString& layer_name) const
{
  const QString session_stem =
      !_session_data_files.isEmpty() ? QFileInfo(_session_data_files.front()).completeBaseName() :
                                       QString();
  if (session_stem.isEmpty())
  {
    return sanitizeAnnotationVariant(layer_name);
  }

  const QString annotation_prefix = session_stem + ".annotations";
  QString variant = layer_name.trimmed();

  if (variant.compare(annotation_prefix, Qt::CaseInsensitive) == 0)
  {
    return annotation_prefix;
  }

  if (variant.startsWith(annotation_prefix, Qt::CaseInsensitive))
  {
    variant = variant.mid(annotation_prefix.size());
  }

  variant.replace(QRegularExpression("^[\\s._()-]+"), "");
  variant = sanitizeAnnotationVariant(variant);

  if (variant.isEmpty())
  {
    return annotation_prefix;
  }
  return annotation_prefix + "." + variant;
}

QString AnnotationsPanel::suggestUniqueAnnotationFilePath() const
{
  const QDir dir(defaultAnnotationDirectory());
  const QString layer_name =
      _manager->activeLayer() && !_manager->activeLayer()->name.trimmed().isEmpty() ?
          _manager->activeLayer()->name.trimmed() :
          suggestUniqueLayerName();
  const QString base = !_session_data_files.isEmpty() ? autoloadSafeAnnotationBaseName(layer_name) :
                                                        sanitizeAnnotationVariant(layer_name);

  QString candidate = base;
  if (dir.exists(candidate + ".json"))
  {
    candidate = nextCopyFileBaseName(base, dir);
  }

  return dir.filePath(candidate + ".json");
}

QString AnnotationsPanel::nextCopyLayerName(const QString& base_name,
                                        const QSet<QString>& used_names) const
{
  QString root = stripTrailingCopySuffix(base_name);
  if (root.isEmpty())
  {
    root = "annotations";
  }

  QString candidate = root + " copy";
  int suffix = 2;
  while (used_names.contains(candidate))
  {
    candidate = QString("%1 copy %2").arg(root).arg(suffix++);
  }
  return candidate;
}

QString AnnotationsPanel::nextCopyFileBaseName(const QString& base_name, const QDir& dir) const
{
  QString root = base_name;
  root.remove(QRegularExpression("\\.copy(?:-\\d+)?$", QRegularExpression::CaseInsensitiveOption));
  root = root.trimmed();
  if (root.isEmpty())
  {
    root = "annotations";
  }

  QString candidate = root + ".copy";
  int suffix = 2;
  while (dir.exists(candidate + ".json"))
  {
    candidate = QString("%1.copy-%2").arg(root).arg(suffix++);
  }
  return candidate;
}
