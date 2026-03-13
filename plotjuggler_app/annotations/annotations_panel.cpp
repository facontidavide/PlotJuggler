#include "annotations_panel.h"
#include "ui_annotations_panel.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>
#include <QInputDialog>
#include <QKeyEvent>
#include <QClipboard>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
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
#include <functional>
#include <limits>

namespace
{
constexpr int COLUMN_ON = 0;
constexpr int COLUMN_NAME = 1;
constexpr int COLUMN_ACTIONS = 2;

constexpr int ROLE_LAYER_INDEX = Qt::UserRole;
constexpr int ROLE_NODE_PATH = Qt::UserRole + 2;
constexpr const char* MIME_ANNOTATION_NODE = "application/x-plotjuggler-annotation-node+json";

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

void styleTreeActionButton(QToolButton* button, bool icon_only = false)
{
  if (!button)
  {
    return;
  }
  button->setAutoRaise(true);
  button->setCursor(Qt::PointingHandCursor);
  button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  button->setMinimumHeight(16);
  button->setMinimumWidth(icon_only ? 16 : 20);
  button->setStyleSheet(
      "QToolButton {"
      " padding: 0 2px;"
      " border: 1px solid transparent;"
      " border-radius: 2px;"
      " background: transparent;"
      " }"
      "QToolButton:hover {"
      " border-color: palette(mid);"
      " background: palette(base);"
      " }"
      "QToolButton:pressed {"
      " border-color: palette(dark);"
      " background: palette(midlight);"
      " }"
      "QToolButton:disabled {"
      " color: palette(mid);"
      " border-color: transparent;"
      " background: transparent;"
      " }");
}

QString stripTrailingCopySuffix(QString text)
{
  text = text.trimmed();
  text.remove(QRegularExpression("\\s+copy(?:\\s+\\d+)?$", QRegularExpression::CaseInsensitiveOption));
  return text.trimmed();
}

QJsonObject annotationItemToJson(const AnnotationManager::AnnotationItem& item)
{
  QJsonObject obj;
  obj["enabled"] = item.enabled;
  obj["type"] = (item.type == AnnotationManager::AnnotationType::Point) ? "point" : "region";
  obj["start"] = item.start_time;
  obj["end"] = item.end_time;
  obj["label"] = item.label;
  obj["tags"] = item.tags;
  obj["notes"] = item.notes;
  obj["color"] = item.color.name(QColor::HexRgb);
  return obj;
}

AnnotationManager::AnnotationItem annotationItemFromJson(const QJsonObject& obj)
{
  AnnotationManager::AnnotationItem item;
  item.enabled = obj.value("enabled").toBool(true);
  item.type = (obj.value("type").toString("point") == "region") ? AnnotationManager::AnnotationType::Region :
                                                                   AnnotationManager::AnnotationType::Point;
  item.start_time = obj.value("start").toDouble();
  item.end_time = obj.contains("end") ? obj.value("end").toDouble(item.start_time) : item.start_time;
  item.label = obj.value("label").toString();
  item.tags = obj.value("tags").toString();
  item.notes = obj.value("notes").toString();
  const auto color = QColor(obj.value("color").toString());
  if (color.isValid())
  {
    item.color = color;
  }
  return item;
}

QJsonObject annotationNodeToJson(const AnnotationManager::AnnotationLayer::AnnotationNode& node)
{
  QJsonObject obj;
  obj["node_type"] =
      (node.type == AnnotationManager::AnnotationLayer::NodeType::Group) ? "group" : "annotation";
  obj["name"] = node.name;
  obj["visible"] = node.visible;
  if (node.color.isValid())
  {
    obj["color"] = node.color.name(QColor::HexRgb);
  }
  if (node.type == AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    QJsonArray children;
    for (const auto& child : node.children)
    {
      children.push_back(annotationNodeToJson(child));
    }
    obj["children"] = children;
  }
  else
  {
    obj["annotation"] = annotationItemToJson(node.annotation);
  }
  return obj;
}

std::optional<AnnotationManager::AnnotationLayer::AnnotationNode> annotationNodeFromJson(
    const QJsonObject& obj)
{
  AnnotationManager::AnnotationLayer::AnnotationNode node;
  const QString node_type = obj.value("node_type").toString("annotation");
  node.type = (node_type == "group") ? AnnotationManager::AnnotationLayer::NodeType::Group :
                                       AnnotationManager::AnnotationLayer::NodeType::Annotation;
  node.name = obj.value("name").toString();
  node.visible = obj.value("visible").toBool(true);
  const auto color = QColor(obj.value("color").toString());
  if (color.isValid())
  {
    node.color = color;
  }
  if (node.type == AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    const auto children = obj.value("children").toArray();
    for (const auto& child_value : children)
    {
      if (!child_value.isObject())
      {
        continue;
      }
      auto child = annotationNodeFromJson(child_value.toObject());
      if (child.has_value())
      {
        node.children.push_back(child.value());
      }
    }
  }
  else if (obj.contains("annotation") && obj.value("annotation").isObject())
  {
    node.annotation = annotationItemFromJson(obj.value("annotation").toObject());
    if (node.name.isEmpty())
    {
      node.name = node.annotation.label;
    }
  }
  else
  {
    return std::nullopt;
  }
  return node;
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
  ui->treeWidgetAnnotations->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->treeWidgetAnnotations->setUniformRowHeights(true);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  ui->treeWidgetAnnotations->header()->setStretchLastSection(false);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_NAME, QHeaderView::Stretch);
  ui->treeWidgetAnnotations->header()->setSectionResizeMode(COLUMN_ACTIONS,
                                                            QHeaderView::ResizeToContents);
  ui->treeWidgetAnnotations->setContextMenuPolicy(Qt::CustomContextMenu);
  ui->treeWidgetAnnotations->setDragEnabled(true);
  ui->treeWidgetAnnotations->viewport()->setAcceptDrops(true);
  ui->treeWidgetAnnotations->setDropIndicatorShown(true);
  ui->treeWidgetAnnotations->setDragDropMode(QAbstractItemView::InternalMove);
  ui->treeWidgetAnnotations->installEventFilter(this);
  ui->groupDetails->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
  ui->buttonNewLayer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  ui->buttonLoadLayer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  ui->buttonGenerateLayer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  _annotation_text_edit_timer = new QTimer(this);
  _annotation_text_edit_timer->setSingleShot(true);
  _annotation_text_edit_timer->setInterval(250);

  connect(ui->buttonNewLayer, &QPushButton::clicked, this, &AnnotationsPanel::onNewLayer);
  connect(ui->buttonLoadLayer, &QPushButton::clicked, this, &AnnotationsPanel::onLoadLayer);
  connect(ui->buttonGenerateLayer, &QPushButton::clicked, this,
          [this]() { emit generateAnnotationsRequested(-1, QString()); });
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
            const int layer_index = layerIndexFromItem(item);
            const QString node_path = nodePathFromItem(item);
            const auto* node = (layer_index >= 0 && !node_path.isEmpty()) ?
                                   _manager->nodeAtPath(layer_index, node_path) :
                                   nullptr;
            if (layer_index >= 0 && annotationRowFromItem(item) < 0 && !node)
            {
              onEditLayerProperties();
            }
            else if (node && node->type == AnnotationManager::AnnotationLayer::NodeType::Group)
            {
              editGroupPropertiesDialog(layer_index, node_path);
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
  connect(ui->treeWidgetAnnotations->model(), &QAbstractItemModel::rowsMoved, this,
          [this]() {
            if (_updating_ui)
            {
              return;
            }
            syncTreeStructureToManager();
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

bool AnnotationsPanel::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == ui->treeWidgetAnnotations && event->type() == QEvent::KeyPress)
  {
    auto* key_event = static_cast<QKeyEvent*>(event);
    if (key_event->matches(QKeySequence::Copy))
    {
      copyCurrentNodeToClipboard();
      return true;
    }
    if (key_event->matches(QKeySequence::Cut))
    {
      cutCurrentNodeToClipboard();
      return true;
    }
    if (key_event->matches(QKeySequence::Paste))
    {
      pasteNodeFromClipboard();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

bool AnnotationsPanel::hasSelectedAnnotation() const
{
  return selectedAnnotationRow() >= 0 && isCurrentSelectionCompatible();
}

AnnotationManager::AnnotationItem AnnotationsPanel::selectedAnnotation() const
{
  const int layer_index = _detail_layer_index;
  if (_detail_annotation_row < 0 || layer_index < 0)
  {
    return {};
  }
  const auto* node = _manager->nodeAtPath(layer_index, nodePathFromItem(currentTreeItem()));
  if (!node || node->type != AnnotationManager::AnnotationLayer::NodeType::Annotation)
  {
    return {};
  }
  return node->annotation;
}

int AnnotationsPanel::selectedAnnotationRow() const
{
  if (_detail_layer_index < 0 || _detail_annotation_row < 0)
  {
    return -1;
  }
  return _manager->flatItemIndexForNodePath(_detail_layer_index, nodePathFromItem(currentTreeItem()));
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
  _manager->updateLayerNodeAnnotation(_detail_layer_index, nodePathFromItem(currentTreeItem()), item);
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
  const QString node_path = nodePathFromItem(item);

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
        if (node_path.isEmpty())
        {
          _manager->setLayerVisible(layer_index, true);
        }
        return;
      }
      const bool enabled = (state == Qt::Checked);
      if (node_path.isEmpty())
      {
        _manager->setLayerVisible(layer_index, enabled);
        _manager->setLayerItemsEnabled(layer_index, enabled);
      }
      else if (auto* node = _manager->nodeAtPath(layer_index, node_path))
      {
        std::function<void(AnnotationManager::AnnotationLayer::AnnotationNode&)> cascade =
            [&](auto& current) {
              current.visible = enabled;
              if (current.type == AnnotationManager::AnnotationLayer::NodeType::Group)
              {
                for (auto& child : current.children)
                {
                  cascade(child);
                }
              }
              else
              {
                current.annotation.enabled = enabled;
              }
            };
        cascade(*node);
        _manager->rebuildLayerItems(layer_index);
      }

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
  const auto* node = _manager->nodeAtPath(layer_index, node_path);
  if (!node || node->type != AnnotationManager::AnnotationLayer::NodeType::Annotation)
  {
    return;
  }

  if (column == COLUMN_ON)
  {
    auto annotation = node->annotation;
    annotation.enabled = (item->checkState(COLUMN_ON) == Qt::Checked);
    _manager->updateLayerNodeAnnotation(layer_index, node_path, annotation);

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
      if (!nodePathFromItem(parent).isEmpty())
      {
        if (auto* parent_node = _manager->nodeAtPath(layer_index, nodePathFromItem(parent)))
        {
          parent_node->visible = (parent_state != Qt::Unchecked);
          _manager->rebuildLayerItems(layer_index);
        }
      }
      else
      {
        _manager->setLayerVisible(layer_index, parent_state != Qt::Unchecked);
      }
    }
  }
  else if (column == COLUMN_NAME)
  {
    auto annotation = node->annotation;
    annotation.label = item->text(COLUMN_NAME) == "<annotation>" ? QString() : item->text(COLUMN_NAME);
    _manager->updateLayerNodeAnnotation(layer_index, node_path, annotation);
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
  QString parent_path;
  if (_detail_layer_index == _manager->activeLayerIndex())
  {
    parent_path = _manager->parentGroupPath(nodePathFromItem(currentTreeItem()));
  }
  _manager->addItemToLayer(_manager->activeLayerIndex(), item, parent_path);
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
  QString parent_path;
  if (_detail_layer_index == _manager->activeLayerIndex())
  {
    parent_path = _manager->parentGroupPath(nodePathFromItem(currentTreeItem()));
  }
  _manager->addItemToLayer(_manager->activeLayerIndex(), item, parent_path);
}

void AnnotationsPanel::onRemoveItem()
{
  if (_detail_layer_index < 0)
  {
    return;
  }
  _manager->removeLayerNode(_detail_layer_index, nodePathFromItem(currentTreeItem()));
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
  const bool preserve_multi_selection =
      item->isSelected() && ui->treeWidgetAnnotations->selectedItems().size() > 1;
  {
    const QSignalBlocker blocker(ui->treeWidgetAnnotations);
    if (preserve_multi_selection)
    {
      ui->treeWidgetAnnotations->setCurrentItem(item, 0, QItemSelectionModel::NoUpdate);
    }
    else
    {
      ui->treeWidgetAnnotations->setCurrentItem(item);
      item->setSelected(true);
    }
  }
  if (!preserve_multi_selection)
  {
    _detail_layer_index = layerIndexFromItem(item);
    _detail_annotation_row = annotationRowFromItem(item);
  }

  const int layer_index = layerIndexFromItem(item);
  const int annotation_row = annotationRowFromItem(item);
  const QString node_path = nodePathFromItem(item);
  const QPoint global_pos = ui->treeWidgetAnnotations->viewport()->mapToGlobal(pos);
  if (annotation_row >= 0)
  {
    openAnnotationMenu(layer_index, annotation_row, node_path, global_pos);
  }
  else if (layer_index >= 0 && !node_path.isEmpty())
  {
    openGroupMenu(layer_index, node_path, global_pos);
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
  ui->buttonGenerateLayer->setEnabled(has_dataset);
  ui->checkBoxAutoloadCompanionAnnotations->setEnabled(has_dataset);
  ui->treeWidgetAnnotations->setEnabled(has_layer || has_dataset);

  _updating_ui = false;
}

void AnnotationsPanel::updateDetailsPanel()
{
  _updating_ui = true;

  const auto& layers = _manager->layers();
  const bool has_layer = (_detail_layer_index >= 0 && _detail_layer_index < layers.size());
  const auto* node =
      has_layer ? _manager->nodeAtPath(_detail_layer_index, nodePathFromItem(currentTreeItem())) : nullptr;
  const bool has_annotation =
      has_layer && node && node->type == AnnotationManager::AnnotationLayer::NodeType::Annotation;

  ui->groupAnnotationDetails->setEnabled(has_annotation);
  if (has_annotation)
  {
    const auto& annotation = node->annotation;
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
  const QString current_node_path = nodePathFromItem(current);
  const int scroll_x = ui->treeWidgetAnnotations->horizontalScrollBar()->value();
  const int scroll_y = ui->treeWidgetAnnotations->verticalScrollBar()->value();
  QSet<QString> expanded_nodes;
  std::function<void(QTreeWidgetItem*)> collect_expanded = [&](QTreeWidgetItem* tree_item) {
    if (!tree_item)
    {
      return;
    }
    if (tree_item->isExpanded())
    {
      expanded_nodes.insert(QString("%1|%2")
                                .arg(layerIndexFromItem(tree_item))
                                .arg(nodePathFromItem(tree_item)));
    }
    for (int child = 0; child < tree_item->childCount(); ++child)
    {
      collect_expanded(tree_item->child(child));
    }
  };
  for (int i = 0; i < ui->treeWidgetAnnotations->topLevelItemCount(); ++i)
  {
    collect_expanded(ui->treeWidgetAnnotations->topLevelItem(i));
  }

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
    if (restore_item && !current_node_path.isEmpty())
    {
      const auto path = AnnotationManager::pathFromString(current_node_path);
      for (const int index : path)
      {
        if (!restore_item || index < 0 || index >= restore_item->childCount())
        {
          restore_item = nullptr;
          break;
        }
        restore_item = restore_item->child(index);
      }
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
  std::function<void(QTreeWidgetItem*)> restore_expanded = [&](QTreeWidgetItem* tree_item) {
    if (!tree_item)
    {
      return;
    }
    const QString key =
        QString("%1|%2").arg(layerIndexFromItem(tree_item)).arg(nodePathFromItem(tree_item));
    tree_item->setExpanded(expanded_nodes.contains(key));
    for (int child = 0; child < tree_item->childCount(); ++child)
    {
      restore_expanded(tree_item->child(child));
    }
  };
  for (int i = 0; i < ui->treeWidgetAnnotations->topLevelItemCount(); ++i)
  {
    restore_expanded(ui->treeWidgetAnnotations->topLevelItem(i));
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
  auto* add_group = menu.addAction(tr("Add Group"));
  auto* generate = menu.addAction(tr("Generate..."));
  menu.addSeparator();
  auto* copy = menu.addAction(tr("Copy"));
  auto* paste = menu.addAction(tr("Paste"));
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
  add_group->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  generate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
  copy->setShortcut(QKeySequence::Copy);
  paste->setShortcut(QKeySequence::Paste);
  properties->setShortcut(QKeySequence(Qt::Key_F2));
  save->setShortcut(QKeySequence::Save);
  save_as->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  remove->setShortcut(QKeySequence::Delete);

  add_point->setEnabled(editable);
  add_range->setEnabled(editable);
  add_group->setEnabled(editable);
  generate->setEnabled(isLayerCompatible(layer));
  copy->setEnabled(true);
  paste->setEnabled(editable);

  QAction* selected = menu.exec(global_pos);
  if (selected == add_point)
  {
    _manager->setActiveLayerIndex(layer_index);
    ui->treeWidgetAnnotations->setCurrentItem(findTreeItem(layer_index, QString()));
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    onAddPoint();
    selectTreeNodeLater(layer_index, QString::number(insert_index));
  }
  else if (selected == add_range)
  {
    _manager->setActiveLayerIndex(layer_index);
    ui->treeWidgetAnnotations->setCurrentItem(findTreeItem(layer_index, QString()));
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    onAddRegion();
    selectTreeNodeLater(layer_index, QString::number(insert_index));
  }
  else if (selected == add_group)
  {
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    if (_manager->addGroupToLayer(layer_index, suggestGroupName(layer_index, QString())))
    {
      selectTreeNodeLater(layer_index, QString::number(insert_index));
    }
  }
  else if (selected == generate)
  {
    emit generateAnnotationsRequested(layer_index, QString());
  }
  else if (selected == copy)
  {
    copyCurrentNodeToClipboard();
  }
  else if (selected == paste)
  {
    _manager->setActiveLayerIndex(layer_index);
    pasteNodeFromClipboard();
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

void AnnotationsPanel::openGroupMenu(int layer_index, const QString& node_path, const QPoint& global_pos)
{
  const auto* node = _manager->nodeAtPath(layer_index, node_path);
  if (!node || node->type != AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    return;
  }

  const auto& layer = _manager->layers()[layer_index];
  const bool editable = isLayerCompatible(layer) && layer.editable;

  QMenu menu(this);
  auto* add_group = menu.addAction(tr("Add Group"));
  auto* add_point = menu.addAction(tr("Add Point Annotation"));
  auto* add_range = menu.addAction(tr("Add Range Annotation"));
  auto* generate = menu.addAction(tr("Generate..."));
  menu.addSeparator();
  auto* copy = menu.addAction(tr("Copy"));
  auto* cut = menu.addAction(tr("Cut"));
  auto* paste = menu.addAction(tr("Paste"));
  auto* properties = menu.addAction(tr("Group Properties..."));
  auto* remove = menu.addAction(tr("Delete Group"));
  add_group->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
  add_point->setShortcut(QKeySequence(Qt::Key_Insert));
  add_range->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Insert));
  generate->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
  copy->setShortcut(QKeySequence::Copy);
  cut->setShortcut(QKeySequence::Cut);
  properties->setShortcut(QKeySequence(Qt::Key_F2));
  paste->setShortcut(QKeySequence::Paste);
  remove->setShortcut(QKeySequence::Delete);

  add_group->setEnabled(editable);
  add_point->setEnabled(editable);
  add_range->setEnabled(editable);
  generate->setEnabled(isLayerCompatible(layer));
  copy->setEnabled(true);
  cut->setEnabled(editable);
  paste->setEnabled(editable);
  remove->setEnabled(editable);

  QAction* selected = menu.exec(global_pos);
  if (selected == add_group)
  {
    if (_manager->addGroupToLayer(layer_index, suggestGroupName(layer_index, node_path), node_path))
    {
      const auto* group = _manager->nodeAtPath(layer_index, node_path);
      if (group)
      {
        auto path = AnnotationManager::pathFromString(node_path);
        path.push_back(group->children.size() - 1);
        const QString new_path = AnnotationManager::pathToString(path);
        selectTreeNodeLater(layer_index, new_path);
      }
    }
  }
  else if (selected == add_point)
  {
    if (auto* item = findTreeItem(layer_index, node_path))
    {
      ui->treeWidgetAnnotations->setCurrentItem(item);
    }
    const auto* group_before = _manager->nodeAtPath(layer_index, node_path);
    const int insert_index = group_before ? group_before->children.size() : 0;
    AnnotationManager::AnnotationItem item;
    item.type = AnnotationManager::AnnotationType::Point;
    item.start_time = _streaming_active ? _current_time : 0.5 * (_view_start_time + _view_end_time);
    item.end_time = item.start_time;
    item.label = QString("Point @ %1").arg(item.start_time, 0, 'f', 3);
    item.color = node->color.isValid() ? node->color : layer.color;
    if (_manager->addItemToLayer(layer_index, item, node_path))
    {
      auto path = AnnotationManager::pathFromString(node_path);
      path.push_back(insert_index);
      selectTreeNodeLater(layer_index, AnnotationManager::pathToString(path));
    }
  }
  else if (selected == add_range)
  {
    if (auto* item = findTreeItem(layer_index, node_path))
    {
      ui->treeWidgetAnnotations->setCurrentItem(item);
    }
    const auto* group_before = _manager->nodeAtPath(layer_index, node_path);
    const int insert_index = group_before ? group_before->children.size() : 0;
    AnnotationManager::AnnotationItem item;
    item.type = AnnotationManager::AnnotationType::Region;
    const double view_width =
        std::abs(_view_end_time - _view_start_time);
    const double width = (view_width > std::numeric_limits<double>::epsilon()) ?
                             std::max(view_width * 0.1, 1e-9) :
                             1.0;
    const double center =
        _streaming_active ? _current_time : 0.5 * (_view_start_time + _view_end_time);
    item.start_time = center - 0.5 * width;
    item.end_time = center + 0.5 * width;
    item.label = QString("Range @ %1").arg(center, 0, 'f', 3);
    item.color = node->color.isValid() ? node->color : layer.color;
    if (_manager->addItemToLayer(layer_index, item, node_path))
    {
      auto path = AnnotationManager::pathFromString(node_path);
      path.push_back(insert_index);
      selectTreeNodeLater(layer_index, AnnotationManager::pathToString(path));
    }
  }
  else if (selected == generate)
  {
    emit generateAnnotationsRequested(layer_index, node_path);
  }
  else if (selected == copy)
  {
    copyCurrentNodeToClipboard();
  }
  else if (selected == cut)
  {
    cutCurrentNodeToClipboard();
  }
  else if (selected == paste)
  {
    pasteNodeFromClipboard();
  }
  else if (selected == properties)
  {
    editGroupPropertiesDialog(layer_index, node_path);
  }
  else if (selected == remove)
  {
    _manager->removeLayerNode(layer_index, node_path);
  }
}

void AnnotationsPanel::openAnnotationMenu(int layer_index, int annotation_row,
                                          const QString& node_path,
                                          const QPoint& global_pos)
{
  const auto* node = _manager->nodeAtPath(layer_index, node_path);
  if (layer_index < 0 || layer_index >= _manager->layers().size() || annotation_row < 0 || !node ||
      node->type != AnnotationManager::AnnotationLayer::NodeType::Annotation)
  {
    return;
  }

  QMenu menu(this);
  auto* jump = menu.addAction(tr("Jump To Annotation"));
  auto* copy = menu.addAction(tr("Copy"));
  auto* cut = menu.addAction(tr("Cut"));
  auto* paste = menu.addAction(tr("Paste"));
  auto* duplicate = menu.addAction(tr("Duplicate Annotation"));
  auto* remove = menu.addAction(tr("Delete Annotation"));
  jump->setShortcut(QKeySequence(Qt::Key_Return));
  copy->setShortcut(QKeySequence::Copy);
  cut->setShortcut(QKeySequence::Cut);
  paste->setShortcut(QKeySequence::Paste);
  duplicate->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
  remove->setShortcut(QKeySequence::Delete);

  QAction* selected = menu.exec(global_pos);
  if (selected == jump)
  {
    _manager->setActiveLayerIndex(layer_index);
    emit jumpToSelectedAnnotationRequested();
  }
  else if (selected == copy)
  {
    copyCurrentNodeToClipboard();
  }
  else if (selected == cut)
  {
    cutCurrentNodeToClipboard();
  }
  else if (selected == paste)
  {
    pasteNodeFromClipboard();
  }
  else if (selected == duplicate && isActiveLayerEditable())
  {
    _manager->setActiveLayerIndex(layer_index);
    auto item = node->annotation;
    if (!item.label.isEmpty())
    {
      item.label += " copy";
    }
    _manager->addItemToLayer(layer_index, item, _manager->parentGroupPath(node_path));
  }
  else if (selected == remove)
  {
    _manager->removeLayerNode(layer_index, node_path);
  }
}

void AnnotationsPanel::installTreeShortcuts()
{
  auto* delete_shortcut = new QShortcut(QKeySequence::Delete, ui->treeWidgetAnnotations);
  connect(delete_shortcut, &QShortcut::activated, this, [this]() {
    const auto items = selectedTreeItemsForNodeOps();
    if (!items.isEmpty())
    {
      for (auto it = items.rbegin(); it != items.rend(); ++it)
      {
        const int layer_index = layerIndexFromItem(*it);
        const QString node_path = nodePathFromItem(*it);
        if (layer_index >= 0 && !node_path.isEmpty())
        {
          _manager->removeLayerNode(layer_index, node_path);
        }
      }
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
    else if (_detail_layer_index >= 0)
    {
      const QString node_path = nodePathFromItem(currentTreeItem());
      if (!node_path.isEmpty())
      {
        editGroupPropertiesDialog(_detail_layer_index, node_path);
      }
      else
      {
        onEditLayerProperties();
      }
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
      const QString node_path = nodePathFromItem(currentTreeItem());
      const auto* node = _manager->nodeAtPath(layer_index, node_path);
      if (layer_index >= 0 && node &&
          node->type == AnnotationManager::AnnotationLayer::NodeType::Annotation)
      {
        auto item = node->annotation;
        if (!item.label.isEmpty())
        {
          item.label += " copy";
        }
        _manager->addItemToLayer(layer_index, item, _manager->parentGroupPath(node_path));
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

  auto* add_group_shortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), ui->treeWidgetAnnotations);
  connect(add_group_shortcut, &QShortcut::activated, this, [this]() {
    if (_detail_layer_index < 0)
    {
      return;
    }
    const QString node_path = nodePathFromItem(currentTreeItem());
    if (!node_path.isEmpty())
    {
      if (_manager->addGroupToLayer(_detail_layer_index,
                                    suggestGroupName(_detail_layer_index, node_path), node_path))
      {
        const auto* group = _manager->nodeAtPath(_detail_layer_index, node_path);
        if (group)
        {
          auto path = AnnotationManager::pathFromString(node_path);
          path.push_back(group->children.size() - 1);
          const QString new_path = AnnotationManager::pathToString(path);
          selectTreeNodeLater(_detail_layer_index, new_path);
        }
      }
    }
    else
    {
      const int insert_index = _manager->layers()[_detail_layer_index].nodes.size();
      if (_manager->addGroupToLayer(_detail_layer_index,
                                    suggestGroupName(_detail_layer_index, QString())))
      {
        selectTreeNodeLater(_detail_layer_index, QString::number(insert_index));
      }
    }
  });

  auto* generate_shortcut =
      new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), ui->treeWidgetAnnotations);
  connect(generate_shortcut, &QShortcut::activated, this, [this]() {
    if (_detail_layer_index < 0)
    {
      return;
    }
    const QString node_path = nodePathFromItem(currentTreeItem());
    emit generateAnnotationsRequested(_detail_layer_index, node_path);
  });
}

void AnnotationsPanel::syncTreeStructureToManager()
{
  if (_updating_ui || _tree_structure_sync_pending)
  {
    return;
  }

  _tree_structure_sync_pending = true;
  QTimer::singleShot(0, this, [this]() {
    _tree_structure_sync_pending = false;
    if (_updating_ui)
    {
      return;
    }

    for (int layer_index = 0; layer_index < ui->treeWidgetAnnotations->topLevelItemCount(); ++layer_index)
    {
      auto* layer_item = ui->treeWidgetAnnotations->topLevelItem(layer_index);
      if (!layer_item)
      {
        continue;
      }

      QVector<AnnotationManager::AnnotationLayer::AnnotationNode> nodes;
      for (int child = 0; child < layer_item->childCount(); ++child)
      {
        if (auto node = buildNodeFromTreeItem(layer_item->child(child)); node.has_value())
        {
          nodes.push_back(node.value());
        }
      }
      _manager->setLayerNodes(layerIndexFromItem(layer_item), nodes);
    }
  });
}

QList<QTreeWidgetItem*> AnnotationsPanel::selectedTreeItems() const
{
  return ui->treeWidgetAnnotations->selectedItems();
}

QList<QTreeWidgetItem*> AnnotationsPanel::selectedTreeItemsForNodeOps() const
{
  auto items = selectedTreeItems();
  if (items.isEmpty())
  {
    if (auto* current = currentTreeItem())
    {
      items.push_back(current);
    }
  }

  QList<QTreeWidgetItem*> filtered;
  QSet<QString> seen_paths;
  for (auto* item : items)
  {
    if (!item)
    {
      continue;
    }
    const int layer_index = layerIndexFromItem(item);
    const QString node_path = nodePathFromItem(item);
    if (layer_index < 0 || node_path.isEmpty())
    {
      continue;
    }

    bool covered_by_parent = false;
    QString parent_path = node_path;
    while (!parent_path.isEmpty())
    {
      parent_path = _manager->parentGroupPath(parent_path);
      if (!parent_path.isEmpty() &&
          seen_paths.contains(QString("%1|%2").arg(layer_index).arg(parent_path)))
      {
        covered_by_parent = true;
        break;
      }
    }
    if (covered_by_parent)
    {
      continue;
    }

    seen_paths.insert(QString("%1|%2").arg(layer_index).arg(node_path));
    filtered.push_back(item);
  }
  return filtered;
}

QList<QTreeWidgetItem*> AnnotationsPanel::selectedTreeItemsForClipboard() const
{
  auto items = selectedTreeItems();
  if (items.isEmpty())
  {
    if (auto* current = currentTreeItem())
    {
      items.push_back(current);
    }
  }

  QList<QTreeWidgetItem*> filtered;
  QSet<QString> seen_paths;
  for (auto* item : items)
  {
    if (!item)
    {
      continue;
    }

    const int layer_index = layerIndexFromItem(item);
    if (layer_index < 0)
    {
      continue;
    }

    const QString node_path = nodePathFromItem(item);
    bool covered_by_parent = false;
    QString parent_path = node_path;
    while (true)
    {
      if (seen_paths.contains(QString("%1|%2").arg(layer_index).arg(parent_path)))
      {
        covered_by_parent = true;
        break;
      }
      if (parent_path.isEmpty())
      {
        break;
      }
      parent_path = _manager->parentGroupPath(parent_path);
    }
    if (covered_by_parent)
    {
      continue;
    }

    seen_paths.insert(QString("%1|%2").arg(layer_index).arg(node_path));
    filtered.push_back(item);
  }
  return filtered;
}

void AnnotationsPanel::selectTreeNodeLater(int layer_index, const QString& node_path)
{
  QTimer::singleShot(0, this, [this, layer_index, node_path]() {
    if (auto* item = findTreeItem(layer_index, node_path))
    {
      ui->treeWidgetAnnotations->setCurrentItem(item);
      ui->treeWidgetAnnotations->scrollToItem(item);
    }
  });
}

QTreeWidgetItem* AnnotationsPanel::findTreeItem(int layer_index, const QString& node_path) const
{
  if (layer_index < 0 || layer_index >= ui->treeWidgetAnnotations->topLevelItemCount())
  {
    return nullptr;
  }

  auto* item = ui->treeWidgetAnnotations->topLevelItem(layer_index);
  if (!item)
  {
    return nullptr;
  }
  if (node_path.isEmpty())
  {
    return item;
  }

  const auto path = AnnotationManager::pathFromString(node_path);
  for (const int index : path)
  {
    if (!item || index < 0 || index >= item->childCount())
    {
      return nullptr;
    }
    item = item->child(index);
  }
  return item;
}

std::optional<AnnotationManager::AnnotationLayer::AnnotationNode> AnnotationsPanel::buildNodeFromTreeItem(
    const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return std::nullopt;
  }
  const int source_layer_index = layerIndexFromItem(item);
  const QString source_node_path = nodePathFromItem(item);
  if (source_layer_index < 0 || source_node_path.isEmpty())
  {
    return std::nullopt;
  }

  auto node = _manager->copyLayerNode(source_layer_index, source_node_path);
  if (!node.has_value())
  {
    return std::nullopt;
  }

  if (node->type == AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    node->children.clear();
    for (int child = 0; child < item->childCount(); ++child)
    {
      if (auto child_node = buildNodeFromTreeItem(item->child(child)); child_node.has_value())
      {
        node->children.push_back(child_node.value());
      }
    }
  }
  return node;
}

QString AnnotationsPanel::exportNodesAsTsv(const QList<QTreeWidgetItem*>& items) const
{
  QStringList lines;
  lines.push_back("layer\tpath\ttype\tlabel\tstart\tend\ttags\tnotes\tenabled\tvisible\tcolor");

  std::function<void(int, const QString&, const AnnotationManager::AnnotationLayer::AnnotationNode&)> visit =
      [&](int layer_index, const QString& path,
          const AnnotationManager::AnnotationLayer::AnnotationNode& node) {
        const auto& layers = _manager->layers();
        const QString layer_name =
            (layer_index >= 0 && layer_index < layers.size()) ? layers[layer_index].name : QString();
        const QString color =
            node.color.isValid() ? node.color.name(QColor::HexRgb) :
                                   ((layer_index >= 0 && layer_index < layers.size()) ?
                                        layers[layer_index].color.name(QColor::HexRgb) :
                                        QString());
        if (node.type == AnnotationManager::AnnotationLayer::NodeType::Group)
        {
          lines.push_back(QString("%1\t%2\tgroup\t%3\t\t\t\t\t\t%4\t%5")
                              .arg(layer_name, path, node.name, node.visible ? "1" : "0", color));
          for (int i = 0; i < node.children.size(); ++i)
          {
            const QString child_path = path.isEmpty() ? QString::number(i) :
                                                        QString("%1/%2").arg(path).arg(i);
            visit(layer_index, child_path, node.children[i]);
          }
          return;
        }

        const auto& a = node.annotation;
        QString notes = a.notes;
        notes.replace('\n', "\\n");
        lines.push_back(QString("%1\t%2\tannotation\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10")
                            .arg(layer_name, path, a.label,
                                 QString::number(a.start_time, 'f', 6),
                                 QString::number(a.end_time, 'f', 6),
                                 a.tags,
                                 notes,
                                 a.enabled ? "1" : "0",
                                 node.visible ? "1" : "0",
                                 color));
      };

  for (auto* item : items)
  {
    const int layer_index = layerIndexFromItem(item);
    const QString node_path = nodePathFromItem(item);
    if (layer_index < 0 || layer_index >= _manager->layers().size())
    {
      continue;
    }
    if (node_path.isEmpty())
    {
      const auto& layer = _manager->layers()[layer_index];
      for (int i = 0; i < layer.nodes.size(); ++i)
      {
        visit(layer_index, QString::number(i), layer.nodes[i]);
      }
      continue;
    }

    const auto* node = _manager->nodeAtPath(layer_index, node_path);
    if (!node)
    {
      continue;
    }
    visit(layer_index, node_path, *node);
  }
  return lines.join('\n');
}

bool AnnotationsPanel::importNodesFromTsv(const QString& tsv_text)
{
  const auto lines = tsv_text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
  if (lines.size() < 2)
  {
    return false;
  }

  const auto headers = lines.front().split('\t');
  QHash<QString, int> column_index;
  for (int i = 0; i < headers.size(); ++i)
  {
    column_index.insert(headers[i].trimmed().toLower(), i);
  }
  if (!column_index.contains("type"))
  {
    return false;
  }

  int target_layer_index = _detail_layer_index >= 0 ? _detail_layer_index : _manager->activeLayerIndex();
  if (target_layer_index < 0)
  {
    return false;
  }

  QString target_parent_path;
  const QString current_path = nodePathFromItem(currentTreeItem());
  if (!current_path.isEmpty())
  {
    const auto* current_node = _manager->nodeAtPath(target_layer_index, current_path);
    if (current_node && current_node->type == AnnotationManager::AnnotationLayer::NodeType::Group)
    {
      target_parent_path = current_path;
    }
    else
    {
      target_parent_path = _manager->parentGroupPath(current_path);
    }
  }

  bool inserted_any = false;
  for (int line_index = 1; line_index < lines.size(); ++line_index)
  {
    const auto columns = lines[line_index].split('\t');
    const QString type = columns.value(column_index.value("type")).trimmed().toLower();
    if (type == "group")
    {
      continue;
    }
    if (type != "annotation" && type != "point" && type != "region")
    {
      continue;
    }

    AnnotationManager::AnnotationItem item;
    item.type = (type == "region") ? AnnotationManager::AnnotationType::Region :
                                     AnnotationManager::AnnotationType::Point;
    item.label = columns.value(column_index.value("label")).trimmed();
    bool start_ok = false;
    item.start_time = columns.value(column_index.value("start")).toDouble(&start_ok);
    if (!start_ok)
    {
      item.start_time = 0.0;
    }
    bool end_ok = false;
    item.end_time = columns.value(column_index.value("end")).toDouble(&end_ok);
    if (!end_ok)
    {
      item.end_time = item.start_time;
    }
    item.tags = columns.value(column_index.value("tags"));
    item.notes = columns.value(column_index.value("notes")).replace("\\n", "\n");
    item.enabled = !column_index.contains("enabled") ||
                   columns.value(column_index.value("enabled")).trimmed() != "0";
    if (column_index.contains("color"))
    {
      const auto color = QColor(columns.value(column_index.value("color")).trimmed());
      if (color.isValid())
      {
        item.color = color;
      }
    }
    inserted_any |= _manager->addItemToLayer(target_layer_index, item, target_parent_path);
  }
  return inserted_any;
}

void AnnotationsPanel::copyCurrentNodeToClipboard()
{
  const auto items = selectedTreeItemsForClipboard();
  if (items.isEmpty())
  {
    return;
  }

  QJsonArray json = QJsonArray();
  for (auto* item : items)
  {
    const int layer_index = layerIndexFromItem(item);
    const QString node_path = nodePathFromItem(item);
    if (layer_index < 0 || layer_index >= _manager->layers().size())
    {
      continue;
    }

    if (node_path.isEmpty())
    {
      const auto& layer = _manager->layers()[layer_index];
      for (const auto& child : layer.nodes)
      {
        json.push_back(annotationNodeToJson(child));
      }
      continue;
    }

    auto node = _manager->copyLayerNode(layer_index, node_path);
    if (node.has_value())
    {
      json.push_back(annotationNodeToJson(node.value()));
    }
  }
  if (json.isEmpty())
  {
    return;
  }

  auto* mime_data = new QMimeData();
  const auto compact = QJsonDocument(json).toJson(QJsonDocument::Compact);
  mime_data->setData(MIME_ANNOTATION_NODE, compact);
  mime_data->setData("application/json", compact);
  mime_data->setText(exportNodesAsTsv(items));
  QApplication::clipboard()->setMimeData(mime_data);
}

void AnnotationsPanel::cutCurrentNodeToClipboard()
{
  copyCurrentNodeToClipboard();
  const auto items = selectedTreeItemsForNodeOps();
  for (auto it = items.rbegin(); it != items.rend(); ++it)
  {
    const int layer_index = layerIndexFromItem(*it);
    const QString node_path = nodePathFromItem(*it);
    if (layer_index >= 0 && !node_path.isEmpty())
    {
      _manager->removeLayerNode(layer_index, node_path);
    }
  }
}

void AnnotationsPanel::pasteNodeFromClipboard()
{
  const auto* mime_data = QApplication::clipboard()->mimeData();
  QByteArray payload = mime_data->data(MIME_ANNOTATION_NODE);
  if (payload.isEmpty())
  {
    payload = mime_data->data("application/json");
  }
  if (payload.isEmpty() && !mime_data->text().isEmpty())
  {
    payload = mime_data->text().toUtf8();
  }
  if (payload.isEmpty())
  {
    return;
  }

  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(payload, &error);
  if (error.error != QJsonParseError::NoError)
  {
    importNodesFromTsv(QString::fromUtf8(payload));
    return;
  }

  auto* item = currentTreeItem();
  int target_layer_index = layerIndexFromItem(item);
  QString target_parent_path;
  if (target_layer_index < 0)
  {
    target_layer_index = _manager->activeLayerIndex();
  }
  if (target_layer_index < 0)
  {
    return;
  }

  const QString current_path = nodePathFromItem(item);
  if (!current_path.isEmpty())
  {
    const auto* current_node = _manager->nodeAtPath(target_layer_index, current_path);
    if (current_node && current_node->type == AnnotationManager::AnnotationLayer::NodeType::Group)
    {
      target_parent_path = current_path;
    }
    else
    {
      target_parent_path = _manager->parentGroupPath(current_path);
    }
  }

  if (document.isArray())
  {
    for (const auto& value : document.array())
    {
      if (!value.isObject())
      {
        continue;
      }
      auto node = annotationNodeFromJson(value.toObject());
      if (node.has_value())
      {
        _manager->insertLayerNode(target_layer_index, target_parent_path, node.value());
      }
    }
    return;
  }

  if (document.isObject())
  {
    auto node = annotationNodeFromJson(document.object());
    if (node.has_value())
    {
      _manager->insertLayerNode(target_layer_index, target_parent_path, node.value());
      return;
    }
  }

  importNodesFromTsv(QString::fromUtf8(payload));
}

void AnnotationsPanel::addLayerItem(int layer_index, const AnnotationManager::AnnotationLayer& layer)
{
  auto* layer_item = new QTreeWidgetItem(ui->treeWidgetAnnotations);
  layer_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
  layer_item->setData(COLUMN_NAME, ROLE_NODE_PATH, QString());
  layer_item->setFlags((layer_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate |
                        Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled) &
                       ~Qt::ItemIsDragEnabled);
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
  int total_children = 0;

  auto* layer_actions = new QWidget(ui->treeWidgetAnnotations);
  auto* layer_layout = new QHBoxLayout(layer_actions);
  layer_layout->setContentsMargins(0, 0, 0, 0);
  layer_layout->setSpacing(2);
  const auto make_text_button = [&](QWidget* parent, const QString& text, const QString& tooltip) {
    auto* button = new QToolButton(parent);
    button->setText(text);
    button->setToolTip(tooltip);
    styleTreeActionButton(button);
    return button;
  };

  auto* add_point_button =
      make_text_button(layer_actions, "+P", tr("Add point annotation to this file"));
  add_point_button->setEnabled(layer.editable && compatible);
  connect(add_point_button, &QToolButton::clicked, this, [this, layer_item, layer_index]() {
    ui->treeWidgetAnnotations->setCurrentItem(layer_item);
    _manager->setActiveLayerIndex(layer_index);
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    onAddPoint();
    selectTreeNodeLater(layer_index, QString::number(insert_index));
  });
  layer_layout->addWidget(add_point_button);

  auto* add_range_button =
      make_text_button(layer_actions, "+R", tr("Add range annotation to this file"));
  add_range_button->setEnabled(layer.editable && compatible);
  connect(add_range_button, &QToolButton::clicked, this, [this, layer_item, layer_index]() {
    ui->treeWidgetAnnotations->setCurrentItem(layer_item);
    _manager->setActiveLayerIndex(layer_index);
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    onAddRegion();
    selectTreeNodeLater(layer_index, QString::number(insert_index));
  });
  layer_layout->addWidget(add_range_button);

  auto* add_group_button =
      make_text_button(layer_actions, "+G", tr("Add group to this file"));
  add_group_button->setEnabled(layer.editable && compatible);
  connect(add_group_button, &QToolButton::clicked, this, [this, layer_index]() {
    _manager->setActiveLayerIndex(layer_index);
    const int insert_index = _manager->layers()[layer_index].nodes.size();
    if (_manager->addGroupToLayer(layer_index, suggestGroupName(layer_index, QString())))
    {
      selectTreeNodeLater(layer_index, QString::number(insert_index));
    }
  });
  layer_layout->addWidget(add_group_button);

  auto* properties_button =
      make_text_button(layer_actions, "Prop", tr("File properties"));
  properties_button->setEnabled(compatible);
  connect(properties_button, &QToolButton::clicked, this, [this, layer_index]() {
    editLayerPropertiesDialog(layer_index);
  });
  layer_layout->addWidget(properties_button);

  auto* save_button = new QToolButton(layer_actions);
  save_button->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
  save_button->setToolTip(layer.dirty ? tr("Save annotation file") :
                                        tr("Annotation file is already saved"));
  save_button->setEnabled(compatible && layer.dirty);
  styleTreeActionButton(save_button, true);
  connect(save_button, &QToolButton::clicked, this, [this, layer_item, layer_index]() {
    ui->treeWidgetAnnotations->setCurrentItem(layer_item);
    _manager->setActiveLayerIndex(layer_index);
    onSaveLayer();
  });
  layer_layout->addWidget(save_button);
  ui->treeWidgetAnnotations->setItemWidget(layer_item, COLUMN_ACTIONS, layer_actions);

  for (int index = 0; index < layer.nodes.size(); ++index)
  {
    addNodeItem(layer_item, layer_index, layer, layer.nodes[index], { index }, enabled_children,
                total_children);
  }

  if (!compatible || total_children == 0)
  {
    layer_item->setCheckState(COLUMN_ON, (layer.visible && compatible) ? Qt::Checked : Qt::Unchecked);
  }
  else if (enabled_children == total_children)
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

void AnnotationsPanel::addNodeItem(QTreeWidgetItem* parent_item, int layer_index,
                                   const AnnotationManager::AnnotationLayer& layer,
                                   const AnnotationManager::AnnotationLayer::AnnotationNode& node,
                                   const QVector<int>& node_path, int& enabled_leaf_count,
                                   int& total_leaf_count)
{
  auto* tree_item = new QTreeWidgetItem(parent_item);
  tree_item->setData(COLUMN_NAME, ROLE_LAYER_INDEX, layer_index);
  tree_item->setData(COLUMN_NAME, ROLE_NODE_PATH, AnnotationManager::pathToString(node_path));

  if (node.type == AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    tree_item->setFlags(tree_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate |
                        Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled |
                        Qt::ItemIsDropEnabled);
    tree_item->setText(COLUMN_NAME, node.name.isEmpty() ? tr("Group") : node.name);
    tree_item->setIcon(COLUMN_NAME, colorSwatchIcon(node.color.isValid() ? node.color : layer.color));

    int group_enabled = 0;
    int group_total = 0;
    for (int child_index = 0; child_index < node.children.size(); ++child_index)
    {
      auto child_path = node_path;
      child_path.push_back(child_index);
      addNodeItem(tree_item, layer_index, layer, node.children[child_index], child_path,
                  group_enabled, group_total);
    }

    if (group_total == 0)
    {
      tree_item->setCheckState(COLUMN_ON, node.visible ? Qt::Checked : Qt::Unchecked);
    }
    else if (group_enabled == group_total)
    {
      tree_item->setCheckState(COLUMN_ON, Qt::Checked);
    }
    else if (group_enabled == 0)
    {
      tree_item->setCheckState(COLUMN_ON, Qt::Unchecked);
    }
    else
    {
      tree_item->setCheckState(COLUMN_ON, Qt::PartiallyChecked);
    }
    auto* group_actions = new QWidget(ui->treeWidgetAnnotations);
    auto* group_layout = new QHBoxLayout(group_actions);
    group_layout->setContentsMargins(0, 0, 0, 0);
    group_layout->setSpacing(2);

    const QString group_path = AnnotationManager::pathToString(node_path);
    auto* add_point_button = new QToolButton(group_actions);
    add_point_button->setText("+P");
    add_point_button->setToolTip(tr("Add point annotation to this group"));
    add_point_button->setEnabled(layer.editable && isLayerCompatible(layer));
    styleTreeActionButton(add_point_button);
    connect(add_point_button, &QToolButton::clicked, this,
            [this, tree_item, layer_index, group_path]() {
              _manager->setActiveLayerIndex(layer_index);
              ui->treeWidgetAnnotations->setCurrentItem(tree_item);
              const auto* group_before = _manager->nodeAtPath(layer_index, group_path);
              const int insert_index = group_before ? group_before->children.size() : 0;
              AnnotationManager::AnnotationItem item;
              item.type = AnnotationManager::AnnotationType::Point;
              item.start_time =
                  _streaming_active ? _current_time : 0.5 * (_view_start_time + _view_end_time);
              item.end_time = item.start_time;
              item.label = QString("Point @ %1").arg(item.start_time, 0, 'f', 3);
              if (const auto* active_layer = _manager->activeLayer())
              {
                item.color = active_layer->color;
              }
              if (_manager->addItemToLayer(layer_index, item, group_path))
              {
                auto path = AnnotationManager::pathFromString(group_path);
                path.push_back(insert_index);
                selectTreeNodeLater(layer_index, AnnotationManager::pathToString(path));
              }
            });
    group_layout->addWidget(add_point_button);

    auto* add_range_button = new QToolButton(group_actions);
    add_range_button->setText("+R");
    add_range_button->setToolTip(tr("Add range annotation to this group"));
    add_range_button->setEnabled(layer.editable && isLayerCompatible(layer));
    styleTreeActionButton(add_range_button);
    connect(add_range_button, &QToolButton::clicked, this,
            [this, tree_item, layer_index, group_path]() {
              _manager->setActiveLayerIndex(layer_index);
              ui->treeWidgetAnnotations->setCurrentItem(tree_item);
              const auto* group_before = _manager->nodeAtPath(layer_index, group_path);
              const int insert_index = group_before ? group_before->children.size() : 0;
              AnnotationManager::AnnotationItem item;
              item.type = AnnotationManager::AnnotationType::Region;
              const double view_width = std::abs(_view_end_time - _view_start_time);
              const double width = (view_width > std::numeric_limits<double>::epsilon()) ?
                                       std::max(view_width * 0.1, 1e-9) :
                                       1.0;
              const double center = _streaming_active ? _current_time :
                                                       0.5 * (_view_start_time + _view_end_time);
              item.start_time = center - 0.5 * width;
              item.end_time = center + 0.5 * width;
              item.label = QString("Range @ %1").arg(center, 0, 'f', 3);
              if (const auto* active_layer = _manager->activeLayer())
              {
                item.color = active_layer->color;
              }
              if (_manager->addItemToLayer(layer_index, item, group_path))
              {
                auto path = AnnotationManager::pathFromString(group_path);
                path.push_back(insert_index);
                selectTreeNodeLater(layer_index, AnnotationManager::pathToString(path));
              }
            });
    group_layout->addWidget(add_range_button);

    auto* add_group_button = new QToolButton(group_actions);
    add_group_button->setText("+G");
    add_group_button->setToolTip(tr("Add subgroup"));
    add_group_button->setEnabled(layer.editable && isLayerCompatible(layer));
    styleTreeActionButton(add_group_button);
    connect(add_group_button, &QToolButton::clicked, this,
            [this, tree_item, layer_index, group_path]() {
              _manager->setActiveLayerIndex(layer_index);
              ui->treeWidgetAnnotations->setCurrentItem(tree_item);
              const auto* group_before = _manager->nodeAtPath(layer_index, group_path);
              const int insert_index = group_before ? group_before->children.size() : 0;
              if (_manager->addGroupToLayer(layer_index,
                                            suggestGroupName(layer_index, group_path), group_path))
              {
                auto path = AnnotationManager::pathFromString(group_path);
                path.push_back(insert_index);
                selectTreeNodeLater(layer_index, AnnotationManager::pathToString(path));
              }
            });
    group_layout->addWidget(add_group_button);

    auto* properties_button = new QToolButton(group_actions);
    properties_button->setText("Prop");
    properties_button->setToolTip(tr("Group properties"));
    properties_button->setEnabled(isLayerCompatible(layer));
    styleTreeActionButton(properties_button);
    connect(properties_button, &QToolButton::clicked, this,
            [this, layer_index, group_path]() { editGroupPropertiesDialog(layer_index, group_path); });
    group_layout->addWidget(properties_button);

    auto* remove_button = new QToolButton(group_actions);
    remove_button->setText("Del");
    remove_button->setToolTip(tr("Delete group"));
    remove_button->setEnabled(layer.editable);
    styleTreeActionButton(remove_button);
    connect(remove_button, &QToolButton::clicked, this,
            [this, layer_index, group_path]() { _manager->removeLayerNode(layer_index, group_path); });
    group_layout->addWidget(remove_button);
    ui->treeWidgetAnnotations->setItemWidget(tree_item, COLUMN_ACTIONS, group_actions);

    enabled_leaf_count += group_enabled;
    total_leaf_count += group_total;
    return;
  }

  const auto& annotation = node.annotation;
  const bool in_range = isAnnotationInSessionRange(annotation);
  const QString kind =
      (annotation.type == AnnotationManager::AnnotationType::Point) ? "Point" : "Range";
  const QString timing =
      (annotation.type == AnnotationManager::AnnotationType::Point) ?
          QString::number(annotation.start_time, 'f', 3) :
          QString("%1..%2").arg(annotation.start_time, 0, 'f', 3).arg(annotation.end_time, 0, 'f', 3);

  tree_item->setFlags(tree_item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable |
                      Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
  tree_item->setCheckState(COLUMN_ON, annotation.enabled ? Qt::Checked : Qt::Unchecked);
  QString summary = annotation.label.isEmpty() ? kind : annotation.label;
  if (!in_range)
  {
    summary += tr(" [out of range]");
  }
  tree_item->setText(COLUMN_NAME, summary);
  if (!in_range)
  {
    tree_item->setForeground(COLUMN_NAME, QBrush(QColor("#c2410c")));
  }
  tree_item->setToolTip(
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

  auto* jump_button = new QToolButton(annotation_actions);
  jump_button->setText("Go");
  jump_button->setToolTip(tr("Jump to annotation"));
  jump_button->setEnabled(in_range);
  styleTreeActionButton(jump_button);
  connect(jump_button, &QToolButton::clicked, this,
          [this, layer_index, tree_item]() {
            _manager->setActiveLayerIndex(layer_index);
            ui->treeWidgetAnnotations->setCurrentItem(tree_item);
            onJumpToItem();
          });
  annotation_layout->addWidget(jump_button);

  QString detail_text = timing;
  if (!annotation.tags.trimmed().isEmpty())
  {
    detail_text += QString("  %1").arg(annotation.tags.trimmed());
  }
  auto* detail_label = new QLabel(detail_text, annotation_actions);
  detail_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  detail_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  detail_label->setStyleSheet("color: palette(mid); padding-left: 4px;");
  detail_label->setToolTip(tree_item->toolTip(COLUMN_NAME));
  annotation_layout->addWidget(detail_label);

  ui->treeWidgetAnnotations->setItemWidget(tree_item, COLUMN_ACTIONS, annotation_actions);

  total_leaf_count++;
  if (annotation.enabled)
  {
    enabled_leaf_count++;
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
  const QString node_path = item->data(COLUMN_NAME, ROLE_NODE_PATH).toString();
  if (node_path.isEmpty())
  {
    return -1;
  }
  return _manager->flatItemIndexForNodePath(layerIndexFromItem(item), node_path);
}

QString AnnotationsPanel::nodePathFromItem(const QTreeWidgetItem* item) const
{
  if (!item)
  {
    return {};
  }
  return item->data(COLUMN_NAME, ROLE_NODE_PATH).toString();
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

QString AnnotationsPanel::suggestGroupName(int layer_index, const QString& parent_node_path) const
{
  QSet<QString> used_names;
  if (layer_index >= 0 && layer_index < _manager->layers().size())
  {
    const auto& layer = _manager->layers()[layer_index];
    if (parent_node_path.isEmpty())
    {
      for (const auto& node : layer.nodes)
      {
        if (node.type == AnnotationManager::AnnotationLayer::NodeType::Group)
        {
          used_names.insert(node.name.trimmed());
        }
      }
    }
    else if (const auto* parent = _manager->nodeAtPath(layer_index, parent_node_path);
             parent && parent->type == AnnotationManager::AnnotationLayer::NodeType::Group)
    {
      for (const auto& child : parent->children)
      {
        if (child.type == AnnotationManager::AnnotationLayer::NodeType::Group)
        {
          used_names.insert(child.name.trimmed());
        }
      }
    }
  }

  int suffix = 1;
  QString candidate;
  do
  {
    candidate = QString("Group %1").arg(suffix++);
  } while (used_names.contains(candidate));
  return candidate;
}

bool AnnotationsPanel::editGroupPropertiesDialog(int layer_index, const QString& node_path)
{
  const auto* node = _manager->nodeAtPath(layer_index, node_path);
  if (!node || node->type != AnnotationManager::AnnotationLayer::NodeType::Group)
  {
    return false;
  }

  QDialog dialog(this);
  dialog.setWindowTitle(tr("Group Properties"));
  auto* layout = new QVBoxLayout(&dialog);
  auto* form = new QFormLayout();
  layout->addLayout(form);

  auto* name_edit = new QLineEdit(node->name, &dialog);
  form->addRow(tr("Name"), name_edit);

  QColor chosen_color = node->color;
  auto* color_row = new QWidget(&dialog);
  auto* color_layout = new QHBoxLayout(color_row);
  color_layout->setContentsMargins(0, 0, 0, 0);
  auto* color_button = new QPushButton(tr("Choose Color"), color_row);
  auto* color_swatch = new QLabel(color_row);
  color_swatch->setFixedSize(20, 20);
  const auto update_swatch = [&]() {
    const QString border = QStringLiteral("border: 1px solid palette(mid);");
    color_swatch->setStyleSheet(QString("background-color: %1; %2")
                                    .arg(chosen_color.isValid() ? chosen_color.name() : QString("transparent"))
                                    .arg(border));
  };
  update_swatch();
  connect(color_button, &QPushButton::clicked, &dialog, [&]() {
    const QColor color =
        QColorDialog::getColor(chosen_color, &dialog, tr("Select Group Color"));
    if (color.isValid())
    {
      chosen_color = color;
      update_swatch();
    }
  });
  color_layout->addWidget(color_button);
  color_layout->addWidget(color_swatch);
  color_layout->addStretch(1);
  form->addRow(tr("Color"), color_row);

  auto* buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() != QDialog::Accepted)
  {
    return false;
  }

  _manager->setLayerNodeName(layer_index, node_path, name_edit->text().trimmed());
  if (chosen_color.isValid())
  {
    _manager->setLayerNodeColor(layer_index, node_path, chosen_color);
  }
  return true;
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
