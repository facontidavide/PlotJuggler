#include "annotations_panel.h"
#include "ui_annotations_panel.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QRegularExpression>
#include <QSet>
#include <QSignalBlocker>
#include <QShortcut>
#include <QScrollBar>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
constexpr int COLUMN_ON = 0;
constexpr int COLUMN_NAME = 1;
constexpr int COLUMN_ACTIONS = 2;

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
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

  ui->treeWidgetAnnotations->setRootIsDecorated(true);
  ui->treeWidgetAnnotations->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->treeWidgetAnnotations->setUniformRowHeights(true);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeWidgetAnnotations->header()->setStretchLastSection(false);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_ACTIONS,
                                                            QHeaderView::ResizeToContents);
  ui->treeWidgetAnnotations->setContextMenuPolicy(Qt::CustomContextMenu);
  ui->groupDetails->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  ui->buttonNewLayer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  ui->buttonLoadLayer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  _annotation_text_edit_timer = new QTimer(this);
  _annotation_text_edit_timer->setSingleShot(true);
  _annotation_text_edit_timer->setInterval(250);

  connect(ui->buttonNewLayer, &QPushButton::clicked, this, &AnnotationsPanel::onNewLayer);
  connect(ui->buttonLoadLayer, &QPushButton::clicked, this, &AnnotationsPanel::onLoadLayer);
  connect(ui->checkBoxAutoloadCompanionAnnotations, &QCheckBox::toggled, this,
          &AnnotationsPanel::autoloadCompanionAnnotationsChanged);
  connect(ui->checkBoxAnnotationEnabled, &QCheckBox::toggled, this,
          &AnnotationsPanel::onAnnotationEnabledEdited);
  connect(ui->lineEditAnnotationLabel, &QLineEdit::editingFinished, this,
          &AnnotationsPanel::onAnnotationLabelEdited);
  connect(ui->lineEditAnnotationTags, &QLineEdit::editingFinished, this,
          &AnnotationsPanel::onAnnotationTagsEdited);
  connect(ui->plainTextEditAnnotationNotes, &QPlainTextEdit::textChanged, this,
          &AnnotationsPanel::onAnnotationNotesEdited);
  connect(_annotation_text_edit_timer, &QTimer::timeout, this,
          &AnnotationsPanel::flushPendingAnnotationTextEdits);

  connect(ui->treeWidgetAnnotations, &QTreeWidget::currentItemChanged, this,
          &AnnotationsPanel::onTreeCurrentItemChanged);
  connect(ui->treeWidgetAnnotations, &QTreeWidget::itemChanged, this, &AnnotationsPanel::onTreeItemChanged);
  connect(ui->treeWidgetAnnotations, &QWidget::customContextMenuRequested, this,
          &AnnotationsPanel::onTreeContextMenu);
  connect(ui->treeWidgetAnnotations, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem* item, int column) {
            if (!item)
            {
              return;
            }
            if (layerIndexFromItem(item) >= 0 && annotationRowFromItem(item) < 0)
            {
              onEditLayerProperties();
            }
            else if (annotationRowFromItem(item) >= 0 && column != COLUMN_ON)
            {
              ui->lineEditAnnotationLabel->setFocus();
              ui->lineEditAnnotationLabel->selectAll();
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
  updateDetailsPanel();
  installTreeShortcuts();
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

void AnnotationsPanel::setSessionTimeRange(double start_time, double end_time)
{
  _session_start_time = std::min(start_time, end_time);
  _session_end_time = std::max(start_time, end_time);
  _session_time_range_valid = std::isfinite(_session_start_time) && std::isfinite(_session_end_time) &&
                              _session_start_time <= _session_end_time;
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

void AnnotationsPanel::clearForSessionChange()
{
  _tree_refresh_pending = false;
  _annotation_text_edit_timer->stop();
  _detail_layer_index = -1;
  _detail_annotation_row = -1;
  _updating_ui = true;
  {
    QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
    ui->treeWidgetAnnotations->clear();
  }
  _updating_ui = false;
  updateButtons();
  updateDetailsPanel();
  emit selectedAnnotationChanged(false);
}

bool AnnotationsPanel::hasSelectedAnnotation() const
{
  return selectedAnnotationRow() >= 0 && isCurrentSelectionCompatible();
}

AnnotationManager::AnnotationItem AnnotationsPanel::selectedAnnotation() const
{
  const int row = _detail_annotation_row;
  const int layer_index = _detail_layer_index;
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
  return (_detail_layer_index >= 0 && _detail_annotation_row >= 0) ? _detail_annotation_row : -1;
}

bool AnnotationsPanel::isActiveLayerEditable() const
{
  const auto* layer = _manager->activeLayer();
  return layer && layer->editable && isLayerCompatible(*layer);
}

void AnnotationsPanel::updateSelectedAnnotation(const AnnotationManager::AnnotationItem& item)
{
  if (_detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }

  if (_manager->activeLayerIndex() != _detail_layer_index)
  {
    _manager->setActiveLayerIndex(_detail_layer_index);
  }
  _manager->updateActiveLayerItem(_detail_annotation_row, item);
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
  _annotation_text_edit_timer->stop();
  if (!current)
  {
    _detail_layer_index = -1;
    _detail_annotation_row = -1;
    updateButtons();
    updateDetailsPanel();
    emit selectedAnnotationChanged(false);
    return;
  }
  const int layer_index = layerIndexFromItem(current);
  _detail_layer_index = layer_index;
  _detail_annotation_row = annotationRowFromItem(current);
  if (layer_index >= 0)
  {
    _manager->setActiveLayerIndex(layer_index);
  }
  updateButtons();
  updateDetailsPanel();
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
      const Qt::CheckState state = item->checkState(COLUMN_ON);
      if (state == Qt::PartiallyChecked)
      {
        _manager->setLayerVisible(layer_index, true);
        return;
      }
      const bool enabled = (state == Qt::Checked);
      _manager->setLayerVisible(layer_index, enabled);
      _manager->setLayerItemsEnabled(layer_index, enabled);

      const QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
      for (int child = 0; child < item->childCount(); ++child)
      {
        item->child(child)->setCheckState(COLUMN_ON, enabled ? Qt::Checked : Qt::Unchecked);
      }
    }
    return;
  }

  if (_manager->activeLayerIndex() != layer_index)
  {
    _manager->setActiveLayerIndex(layer_index);
  }
  const auto& layers = _manager->layers();
  if (layer_index < 0 || layer_index >= layers.size() ||
      annotation_row < 0 || annotation_row >= layers[layer_index].items.size())
  {
    return;
  }

  if (column == COLUMN_ON)
  {
    auto annotation = layers[layer_index].items[annotation_row];
    annotation.enabled = (item->checkState(COLUMN_ON) == Qt::Checked);
    _manager->updateActiveLayerItem(annotation_row, annotation);

    if (auto* parent = item->parent())
    {
      int checked_count = 0;
      int unchecked_count = 0;
      for (int child = 0; child < parent->childCount(); ++child)
      {
        const auto state = parent->child(child)->checkState(COLUMN_ON);
        if (state == Qt::Checked)
        {
          checked_count++;
        }
        else
        {
          unchecked_count++;
        }
      }

      Qt::CheckState parent_state = Qt::PartiallyChecked;
      if (checked_count == parent->childCount())
      {
        parent_state = Qt::Checked;
      }
      else if (unchecked_count == parent->childCount())
      {
        parent_state = Qt::Unchecked;
      }

      const QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
      parent->setCheckState(COLUMN_ON, parent_state);
      _manager->setLayerVisible(layer_index, parent_state != Qt::Unchecked);
    }
  }
  else if (column == COLUMN_NAME)
  {
    auto annotation = layers[layer_index].items[annotation_row];
    annotation.label = item->text(COLUMN_NAME) == "<annotation>" ? QString() : item->text(COLUMN_NAME);
    _manager->updateActiveLayerItem(annotation_row, annotation);
  }
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
      this, tr("Load Annotation File"), defaultAnnotationDirectory(),
      tr("Annotation Files (*.annotations*.json *.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->loadLayer(file_path))
  {
    QMessageBox::warning(this, tr("Load failed"), tr("Failed to load annotation file."));
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
    QMessageBox::warning(this, tr("Save failed"), tr("Failed to save annotation file."));
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
      this, tr("Save Annotation File As"), suggestUniqueAnnotationFilePath(),
      tr("Annotation Files (*.annotations*.json);;JSON Files (*.json);;All Files (*)"));
  if (!file_path.isEmpty() && !_manager->saveLayerAs(index, file_path))
  {
    QMessageBox::warning(this, tr("Save failed"), tr("Failed to save annotation file."));
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
  const QString name = QInputDialog::getText(this, tr("Duplicate Annotation File"),
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
  const QString name = QInputDialog::getText(this, tr("Rename Annotation File"), tr("Layer name:"),
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

void AnnotationsPanel::onAddPoint()
{
  AnnotationManager::AnnotationItem item;
  item.type = AnnotationManager::AnnotationType::Point;
  item.start_time = _streaming_active ? _current_time : 0.5 * (_view_start_time + _view_end_time);
  item.end_time = item.start_time;
  item.label = QString("Point @ %1").arg(item.start_time, 0, 'f', 3);

  if (const auto* layer = _manager->activeLayer())
  {
    item.color = layer->color;
  }
  _manager->addItemToActiveLayer(item);
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

void AnnotationsPanel::onTreeContextMenu(const QPoint& pos)
{
  auto* item = ui->treeWidgetAnnotations->itemAt(pos);
  if (!item)
  {
    return;
  }
  {
    const QSignalBlocker blocker(ui->treeWidgetAnnotations);
    ui->treeWidgetAnnotations->setCurrentItem(item);
  }
  _detail_layer_index = layerIndexFromItem(item);
  _detail_annotation_row = annotationRowFromItem(item);

  const int layer_index = layerIndexFromItem(item);
  const int annotation_row = annotationRowFromItem(item);
  const QPoint global_pos = ui->treeWidgetAnnotations->viewport()->mapToGlobal(pos);
  if (annotation_row >= 0)
  {
    openAnnotationMenu(layer_index, annotation_row, global_pos);
  }
  else if (layer_index >= 0)
  {
    openLayerMenu(layer_index, global_pos);
  }
}

void AnnotationsPanel::onEditLayerProperties()
{
  const int layer_index = _manager->activeLayerIndex();
  if (layer_index >= 0)
  {
    editLayerPropertiesDialog(layer_index);
  }
}

void AnnotationsPanel::onAnnotationEnabledEdited(bool checked)
{
  if (_updating_ui || _detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }
  auto item = selectedAnnotation();
  item.enabled = checked;
  updateSelectedAnnotation(item);
  if (auto* tree_item = currentTreeItem())
  {
    const QSignalBlocker blocker(ui->treeWidgetAnnotations);
    tree_item->setCheckState(COLUMN_ON, checked ? Qt::Checked : Qt::Unchecked);
  }
}

void AnnotationsPanel::onAnnotationLabelEdited()
{
  if (_updating_ui || _detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }
  auto item = selectedAnnotation();
  item.label = ui->lineEditAnnotationLabel->text();
  updateSelectedAnnotation(item);
}

void AnnotationsPanel::onAnnotationTagsEdited()
{
  if (_updating_ui || _detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }
  auto item = selectedAnnotation();
  item.tags = ui->lineEditAnnotationTags->text();
  updateSelectedAnnotation(item);
}

void AnnotationsPanel::onAnnotationNotesEdited()
{
  if (_updating_ui || _detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }
  _annotation_text_edit_timer->start();
}

void AnnotationsPanel::flushPendingAnnotationTextEdits()
{
  if (_updating_ui || _detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return;
  }
  auto item = selectedAnnotation();
  item.label = ui->lineEditAnnotationLabel->text();
  item.tags = ui->lineEditAnnotationTags->text();
  item.notes = ui->plainTextEditAnnotationNotes->toPlainText();
  updateSelectedAnnotation(item);
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

  ui->buttonNewLayer->setEnabled(has_dataset);
  ui->buttonLoadLayer->setEnabled(has_dataset);
  ui->checkBoxAutoloadCompanionAnnotations->setEnabled(has_dataset);
  ui->treeWidgetAnnotations->setEnabled(has_layer || has_dataset);

  _updating_ui = false;
}

void AnnotationsPanel::updateDetailsPanel()
{
  _updating_ui = true;

  const auto& layers = _manager->layers();
  const bool has_layer = (_detail_layer_index >= 0 && _detail_layer_index < layers.size());
  const auto* layer = has_layer ? &layers[_detail_layer_index] : nullptr;
  const bool has_annotation =
      has_layer && _detail_annotation_row >= 0 && _detail_annotation_row < layer->items.size();

  ui->groupAnnotationDetails->setEnabled(has_annotation);
  if (has_annotation)
  {
    const auto& annotation = layer->items[_detail_annotation_row];
    if (!ui->lineEditAnnotationLabel->hasFocus() &&
        ui->lineEditAnnotationLabel->text() != annotation.label)
    {
      ui->lineEditAnnotationLabel->setText(annotation.label);
    }
    if (!ui->lineEditAnnotationTags->hasFocus() &&
        ui->lineEditAnnotationTags->text() != annotation.tags)
    {
      ui->lineEditAnnotationTags->setText(annotation.tags);
    }
    if (!ui->plainTextEditAnnotationNotes->hasFocus() &&
        ui->plainTextEditAnnotationNotes->toPlainText() != annotation.notes)
    {
      ui->plainTextEditAnnotationNotes->setPlainText(annotation.notes);
    }
    ui->checkBoxAnnotationEnabled->setChecked(annotation.enabled);
  }
  else
  {
    ui->lineEditAnnotationLabel->clear();
    ui->lineEditAnnotationTags->clear();
    ui->plainTextEditAnnotationNotes->clear();
    ui->checkBoxAnnotationEnabled->setChecked(false);
  }

  _updating_ui = false;
  ui->groupDetails->setMaximumHeight(ui->groupDetails->sizeHint().height());
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
  const int layer_index = _detail_layer_index;
  const auto& layers = _manager->layers();
  if (layer_index < 0 || layer_index >= layers.size())
  {
    return true;
  }
  return isLayerCompatible(layers[layer_index]);
}

bool AnnotationsPanel::isAnnotationInSessionRange(const AnnotationManager::AnnotationItem& item) const
{
  if (!_session_time_range_valid)
  {
    return true;
  }
  return item.end_time >= _session_start_time && item.start_time <= _session_end_time;
}

void AnnotationsPanel::populateTree()
{
  _updating_ui = true;
  auto* current = currentTreeItem();
  const int current_layer = layerIndexFromItem(current);
  const int current_annotation = annotationRowFromItem(current);
  const int scroll_x = ui->treeWidgetAnnotations->horizontalScrollBar()->value();
  const int scroll_y = ui->treeWidgetAnnotations->verticalScrollBar()->value();
  QSet<int> expanded_layers;
  for (int i = 0; i < ui->treeWidgetAnnotations->topLevelItemCount(); ++i)
  {
    if (auto* top_item = ui->treeWidgetAnnotations->topLevelItem(i);
        top_item && top_item->isExpanded())
    {
      expanded_layers.insert(layerIndexFromItem(top_item));
    }
  }

  QSignalBlocker tree_blocker(ui->treeWidgetAnnotations);
  ui->treeWidgetAnnotations->clear();

  const auto& layers = _manager->layers();
  for (int i = 0; i < layers.size(); ++i)
  {
    addLayerItem(i, layers[i]);
    if (auto* top_item = ui->treeWidgetAnnotations->topLevelItem(i))
    {
      top_item->setExpanded(expanded_layers.contains(i));
    }
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
  ui->treeWidgetAnnotations->horizontalScrollBar()->setValue(scroll_x);
  ui->treeWidgetAnnotations->verticalScrollBar()->setValue(scroll_y);

  auto* selected_item = ui->treeWidgetAnnotations->currentItem();
  _detail_layer_index = layerIndexFromItem(selected_item);
  _detail_annotation_row = annotationRowFromItem(selected_item);

  _updating_ui = false;
  updateDetailsPanel();
  updateButtons();
  emit selectedAnnotationChanged(hasSelectedAnnotation());
}

void AnnotationsPanel::openLayerMenu(int layer_index, const QPoint& global_pos)
{
  if (layer_index < 0 || layer_index >= _manager->layers().size())
  {
    return;
  }

  const auto& layer = _manager->layers()[layer_index];
  const bool editable = isLayerCompatible(layer) && layer.editable;

  QMenu menu(this);
  auto* add_point = menu.addAction(tr("Add Point Annotation"));
  auto* add_range = menu.addAction(tr("Add Range Annotation"));
  menu.addSeparator();
  auto* properties = menu.addAction(tr("File Properties..."));
  auto* save = menu.addAction(tr("Save Annotation File"));
  auto* save_as = menu.addAction(tr("Save Annotation File As..."));
  menu.addSeparator();
  auto* duplicate = menu.addAction(tr("Duplicate Layer"));
  auto* rename = menu.addAction(tr("Rename Layer"));
  auto* toggle_editable =
      menu.addAction(layer.editable ? tr("Set Read Only") : tr("Set Editable"));
  auto* remove = menu.addAction(tr("Delete Layer"));
  add_point->setShortcut(QKeySequence(Qt::Key_Insert));
  add_range->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Insert));
  properties->setShortcut(QKeySequence(Qt::Key_F2));
  save->setShortcut(QKeySequence::Save);
  save_as->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  remove->setShortcut(QKeySequence::Delete);

  add_point->setEnabled(editable);
  add_range->setEnabled(editable);

  QAction* selected = menu.exec(global_pos);
  if (selected == add_point)
  {
    _manager->setActiveLayerIndex(layer_index);
    onAddPoint();
  }
  else if (selected == add_range)
  {
    _manager->setActiveLayerIndex(layer_index);
    onAddRegion();
  }
  else if (selected == properties)
  {
    editLayerPropertiesDialog(layer_index);
  }
  else if (selected == save)
  {
    _manager->setActiveLayerIndex(layer_index);
    onSaveLayer();
  }
  else if (selected == save_as)
  {
    _manager->setActiveLayerIndex(layer_index);
    onSaveLayerAs();
  }
  else if (selected == duplicate)
  {
    _manager->setActiveLayerIndex(layer_index);
    onDuplicateLayer();
  }
  else if (selected == rename)
  {
    _manager->setActiveLayerIndex(layer_index);
    onRenameLayer();
  }
  else if (selected == toggle_editable)
  {
    _manager->setLayerEditable(layer_index, !layer.editable);
  }
  else if (selected == remove)
  {
    _manager->setActiveLayerIndex(layer_index);
    onRemoveLayer();
  }
}

void AnnotationsPanel::openAnnotationMenu(int layer_index, int annotation_row,
                                          const QPoint& global_pos)
{
  if (layer_index < 0 || layer_index >= _manager->layers().size() || annotation_row < 0 ||
      annotation_row >= _manager->layers()[layer_index].items.size())
  {
    return;
  }

  QMenu menu(this);
  auto* jump = menu.addAction(tr("Jump To Annotation"));
  auto* duplicate = menu.addAction(tr("Duplicate Annotation"));
  auto* remove = menu.addAction(tr("Delete Annotation"));
  jump->setShortcut(QKeySequence(Qt::Key_Return));
  duplicate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  remove->setShortcut(QKeySequence::Delete);

  QAction* selected = menu.exec(global_pos);
  if (selected == jump)
  {
    _manager->setActiveLayerIndex(layer_index);
    emit jumpToSelectedAnnotationRequested();
  }
  else if (selected == duplicate && isActiveLayerEditable())
  {
    _manager->setActiveLayerIndex(layer_index);
    auto item = _manager->layers()[layer_index].items[annotation_row];
    if (!item.label.isEmpty())
    {
      item.label += " copy";
    }
    _manager->addItemToActiveLayer(item);
  }
  else if (selected == remove)
  {
    _manager->setActiveLayerIndex(layer_index);
    _manager->removeActiveLayerItem(annotation_row);
  }
}

void AnnotationsPanel::installTreeShortcuts()
{
  auto* delete_shortcut = new QShortcut(QKeySequence::Delete, ui->treeWidgetAnnotations);
  connect(delete_shortcut, &QShortcut::activated, this, [this]() {
    if (selectedAnnotationRow() >= 0)
    {
      onRemoveItem();
    }
    else if (_manager->activeLayerIndex() >= 0)
    {
      onRemoveLayer();
    }
  });

  auto* rename_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), ui->treeWidgetAnnotations);
  connect(rename_shortcut, &QShortcut::activated, this, [this]() {
    if (selectedAnnotationRow() >= 0)
    {
      ui->lineEditAnnotationLabel->setFocus();
      ui->lineEditAnnotationLabel->selectAll();
    }
    else
    {
      onEditLayerProperties();
    }
  });

  auto* jump_shortcut = new QShortcut(QKeySequence(Qt::Key_Return), ui->treeWidgetAnnotations);
  connect(jump_shortcut, &QShortcut::activated, this, &AnnotationsPanel::onJumpToItem);

  auto* insert_shortcut = new QShortcut(QKeySequence(Qt::Key_Insert), ui->treeWidgetAnnotations);
  connect(insert_shortcut, &QShortcut::activated, this, &AnnotationsPanel::onAddPoint);

  auto* range_shortcut =
      new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Insert), ui->treeWidgetAnnotations);
  connect(range_shortcut, &QShortcut::activated, this, &AnnotationsPanel::onAddRegion);

  auto* duplicate_shortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), ui->treeWidgetAnnotations);
  connect(duplicate_shortcut, &QShortcut::activated, this, [this]() {
    if (selectedAnnotationRow() >= 0 && isActiveLayerEditable())
    {
      const int layer_index = _manager->activeLayerIndex();
      const int annotation_row = selectedAnnotationRow();
      if (layer_index >= 0 && annotation_row >= 0 &&
          layer_index < _manager->layers().size() &&
          annotation_row < _manager->layers()[layer_index].items.size())
      {
        auto item = _manager->layers()[layer_index].items[annotation_row];
        if (!item.label.isEmpty())
        {
          item.label += " copy";
        }
        _manager->addItemToActiveLayer(item);
      }
    }
    else if (_manager->activeLayerIndex() >= 0)
    {
      onDuplicateLayer();
    }
  });

  auto* save_shortcut = new QShortcut(QKeySequence::Save, ui->treeWidgetAnnotations);
  connect(save_shortcut, &QShortcut::activated, this, &AnnotationsPanel::onSaveLayer);

  auto* save_as_shortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S), ui->treeWidgetAnnotations);
  connect(save_as_shortcut, &QShortcut::activated, this, &AnnotationsPanel::onSaveLayerAs);
}

void AnnotationsPanel::addLayerItem(int layer_index, const AnnotationManager::AnnotationLayer& layer)
{
  auto* layer_item = new QTreeWidgetItem(ui->treeWidgetAnnotations);
  layer_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
  layer_item->setFlags(layer_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate |
                       Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  const bool compatible = isLayerCompatible(layer);

  QString layer_name = layer.name;
  if (layer.dirty)
  {
    layer_name += " *";
  }
  layer_item->setText(COLUMN_NAME, layer_name);
  layer_item->setIcon(COLUMN_NAME, colorSwatchIcon(layer.color));
  layer_item->setToolTip(COLUMN_NAME, layer.file_path.isEmpty() ? layer.name : layer.file_path);
  layer_item->setExpanded(true);

  auto layer_font = layer_item->font(COLUMN_NAME);
  layer_font.setBold(true);
  layer_item->setFont(COLUMN_NAME, layer_font);

  int enabled_children = 0;

  auto* layer_actions = new QWidget(ui->treeWidgetAnnotations);
  auto* layer_layout = new QHBoxLayout(layer_actions);
  layer_layout->setContentsMargins(0, 0, 0, 0);
  layer_layout->setSpacing(2);
  auto* save_button = new QToolButton(layer_actions);
  save_button->setAutoRaise(true);
  save_button->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  save_button->setToolTip(tr("Save annotation file"));
  save_button->setEnabled(compatible);
  connect(save_button, &QToolButton::clicked, this, [this, layer_item, layer_index]() {
    ui->treeWidgetAnnotations->setCurrentItem(layer_item);
    _manager->setActiveLayerIndex(layer_index);
    onSaveLayer();
  });
  layer_layout->addWidget(save_button);
  ui->treeWidgetAnnotations->setItemWidget(layer_item, COLUMN_ACTIONS, layer_actions);

  for (int row = 0; row < layer.items.size(); ++row)
  {
    const auto& annotation = layer.items[row];
    const bool in_range = isAnnotationInSessionRange(annotation);
    const QString kind = (annotation.type == AnnotationManager::AnnotationType::Point) ? "Point" : "Range";
    const QString timing =
        (annotation.type == AnnotationManager::AnnotationType::Point) ?
            QString::number(annotation.start_time, 'f', 3) :
            QString("%1..%2")
                .arg(annotation.start_time, 0, 'f', 3)
                .arg(annotation.end_time, 0, 'f', 3);
    auto* annotation_item = new QTreeWidgetItem(layer_item);
    annotation_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
    annotation_item->setData(COLUMN_NAME, ROLE_ANNOTATION_ROW, row);
    annotation_item->setFlags(annotation_item->flags() | Qt::ItemIsUserCheckable |
                              Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    annotation_item->setCheckState(COLUMN_ON, annotation.enabled ? Qt::Checked : Qt::Unchecked);
    if (annotation.enabled)
    {
      enabled_children++;
    }
    QString summary =
        annotation.label.isEmpty() ? QString("%1 %2").arg(kind, timing) :
                                     QString("%1  %2").arg(annotation.label, timing);
    if (!in_range)
    {
      summary += tr(" [out of range]");
    }
    annotation_item->setText(COLUMN_NAME, summary);
    if (!in_range)
    {
      annotation_item->setForeground(COLUMN_NAME, QBrush(QColor("#c2410c")));
    }
    annotation_item->setToolTip(
        COLUMN_NAME,
        QString("%1\nStart: %2\nEnd: %3\nTags: %4\nNotes: %5%6")
            .arg(annotation.label.isEmpty() ? kind : annotation.label)
            .arg(annotation.start_time, 0, 'f', 6)
            .arg(annotation.end_time, 0, 'f', 6)
            .arg(annotation.tags)
            .arg(annotation.notes)
            .arg(in_range ? QString() :
                            QString("\nWarning: annotation is outside the current session time range")));

    auto* annotation_actions = new QWidget(ui->treeWidgetAnnotations);
    auto* annotation_layout = new QHBoxLayout(annotation_actions);
    annotation_layout->setContentsMargins(0, 0, 0, 0);
    annotation_layout->setSpacing(2);

    auto* range_button = new QToolButton(annotation_actions);
    range_button->setAutoRaise(true);
    range_button->setText("+R");
    range_button->setToolTip(tr("Add range annotation to this file"));
    range_button->setEnabled(layer.editable && compatible);
    connect(range_button, &QToolButton::clicked, this,
            [this, annotation_item, layer_index]() {
              _manager->setActiveLayerIndex(layer_index);
              ui->treeWidgetAnnotations->setCurrentItem(annotation_item);
              onAddRegion();
            });
    annotation_layout->addWidget(range_button);

    auto* jump_button = new QToolButton(annotation_actions);
    jump_button->setAutoRaise(true);
    jump_button->setText("Go");
    jump_button->setToolTip(tr("Jump to annotation"));
    jump_button->setEnabled(in_range);
    connect(jump_button, &QToolButton::clicked, this,
            [this, layer_index, annotation_item]() {
              _manager->setActiveLayerIndex(layer_index);
              ui->treeWidgetAnnotations->setCurrentItem(annotation_item);
              onJumpToItem();
            });
    annotation_layout->addWidget(jump_button);

    ui->treeWidgetAnnotations->setItemWidget(annotation_item, COLUMN_ACTIONS, annotation_actions);
  }

  if (!compatible || layer.items.isEmpty())
  {
    layer_item->setCheckState(COLUMN_ON, (layer.visible && compatible) ? Qt::Checked : Qt::Unchecked);
  }
  else if (enabled_children == layer.items.size())
  {
    layer_item->setCheckState(COLUMN_ON, Qt::Checked);
  }
  else if (enabled_children == 0)
  {
    layer_item->setCheckState(COLUMN_ON, Qt::Unchecked);
  }
  else
  {
    layer_item->setCheckState(COLUMN_ON, Qt::PartiallyChecked);
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
  return "annotations";
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

bool AnnotationsPanel::editLayerPropertiesDialog(int layer_index)
{
  const auto& layers = _manager->layers();
  if (layer_index < 0 || layer_index >= layers.size())
  {
    return false;
  }

  const auto& layer = layers[layer_index];
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Annotation File Properties"));

  auto* layout = new QVBoxLayout(&dialog);
  auto* form = new QFormLayout();
  auto* name_edit = new QLineEdit(layer.name, &dialog);
  auto* file_label = new QLabel(layer.file_path, &dialog);
  auto* axis_label =
      new QLabel(layer.axis_id.isEmpty() ? layer.axis_kind : layer.axis_id, &dialog);
  auto* status_label = new QLabel(layer.editable ? tr("Editable") : tr("Read only"), &dialog);
  auto* visibility_label = new QLabel(layer.visible ? tr("Visible") : tr("Hidden"), &dialog);
  auto* color_button = new QPushButton(tr("Choose Color"), &dialog);
  QColor selected_color = layer.color;

  file_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  axis_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  file_label->setStyleSheet("color: palette(mid);");
  axis_label->setStyleSheet("color: palette(mid);");
  status_label->setStyleSheet("color: palette(mid);");
  visibility_label->setStyleSheet("color: palette(mid);");
  color_button->setIcon(colorSwatchIcon(selected_color));

  form->addRow(tr("Name"), name_edit);
  form->addRow(tr("File"), file_label);
  form->addRow(tr("Axis"), axis_label);
  form->addRow(tr("Status"), status_label);
  form->addRow(tr("Visibility"), visibility_label);
  form->addRow(tr("Color"), color_button);
  layout->addLayout(form);

  connect(color_button, &QPushButton::clicked, &dialog, [&]() {
    const QColor chosen = QColorDialog::getColor(selected_color, &dialog,
                                                 tr("Select Annotation Color"));
    if (chosen.isValid())
    {
      selected_color = chosen;
      color_button->setIcon(colorSwatchIcon(selected_color));
    }
  });

  auto apply_properties = [this, layer_index, name_edit, &selected_color]() {
    const QString trimmed_name = name_edit->text().trimmed();
    if (!trimmed_name.isEmpty())
    {
      _manager->setLayerName(layer_index, trimmed_name);
    }
    _manager->setLayerColor(layer_index, selected_color);
  };

  auto* close_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             Qt::Horizontal, &dialog);
  layout->addWidget(close_buttons);

  connect(close_buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
    apply_properties();
    dialog.accept();
  });
  connect(close_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  return dialog.exec() == QDialog::Accepted;
}
