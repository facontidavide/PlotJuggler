#include "annotation_manager.h"

#include "nlohmann/json.hpp"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <array>
#include <algorithm>
#include <functional>

namespace
{
using json = nlohmann::json;
using AnnotationNode = AnnotationManager::AnnotationLayer::AnnotationNode;
using NodeType = AnnotationManager::AnnotationLayer::NodeType;

QColor defaultLayerColor(int index)
{
  static const std::array<QColor, 6> palette = {
    QColor("#0ea5a4"), QColor("#f59e0b"), QColor("#ef4444"),
    QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#84cc16")
  };
  return palette[static_cast<size_t>(index) % palette.size()];
}

QString annotationTypeToString(AnnotationManager::AnnotationType type)
{
  return (type == AnnotationManager::AnnotationType::Point) ? "point" : "region";
}

AnnotationManager::AnnotationType annotationTypeFromString(const QString& type)
{
  return (type.compare("region", Qt::CaseInsensitive) == 0) ? AnnotationManager::AnnotationType::Region :
                                                               AnnotationManager::AnnotationType::Point;
}

QString axisKindFromId(const QString& axis_id)
{
  if (axis_id == "row_number")
  {
    return "row_number";
  }
  if (axis_id == "sample_number")
  {
    return "sample_number";
  }
  if (axis_id.isEmpty())
  {
    return "unknown";
  }
  return "column";
}

json colorToJson(const QColor& color)
{
  return color.name(QColor::HexRgb).toStdString();
}

QColor colorFromJson(const json& value, const QColor& fallback)
{
  if (!value.is_string())
  {
    return fallback;
  }
  const auto color = QColor(QString::fromStdString(value.get<std::string>()));
  return color.isValid() ? color : fallback;
}

json annotationItemToJson(const AnnotationManager::AnnotationItem& item, const QColor& fallback_color)
{
  json item_json;
  item_json["enabled"] = item.enabled;
  item_json["type"] = annotationTypeToString(item.type).toStdString();
  item_json["start"] = item.start_time;
  item_json["end"] = item.end_time;
  item_json["label"] = item.label.toStdString();
  item_json["tags"] = item.tags.toStdString();
  item_json["notes"] = item.notes.toStdString();
  item_json["color"] = colorToJson(item.color.isValid() ? item.color : fallback_color);
  return item_json;
}

AnnotationManager::AnnotationItem annotationItemFromJson(const json& item_json, const QColor& fallback_color)
{
  AnnotationManager::AnnotationItem item;
  item.enabled = item_json.value("enabled", true);
  item.type = annotationTypeFromString(
      QString::fromStdString(item_json.value("type", std::string("point"))));
  item.start_time = item_json.value("start", item_json.value("start_time", 0.0));
  item.end_time = item_json.value("end", item_json.value("end_time", item.start_time));
  item.label = QString::fromStdString(item_json.value("label", std::string()));
  item.tags = QString::fromStdString(item_json.value("tags", std::string()));
  item.notes = QString::fromStdString(item_json.value("notes", std::string()));
  item.color = item_json.contains("color") ? colorFromJson(item_json["color"], fallback_color) :
                                             fallback_color;
  return item;
}

AnnotationNode annotationNodeFromItem(const AnnotationManager::AnnotationItem& item)
{
  AnnotationNode node;
  node.type = NodeType::Annotation;
  node.visible = true;
  node.color = item.color;
  node.annotation = item;
  node.name = item.label;
  return node;
}

AnnotationNode annotationNodeFromJson(const json& node_json, const QColor& fallback_color)
{
  AnnotationNode node;
  const QString type = QString::fromStdString(node_json.value("type", std::string("annotation")));
  node.type = (type.compare("group", Qt::CaseInsensitive) == 0) ? NodeType::Group :
                                                                  NodeType::Annotation;
  node.name = QString::fromStdString(node_json.value("name", std::string()));
  node.visible = node_json.value("visible", true);
  node.color = node_json.contains("color") ? colorFromJson(node_json["color"], fallback_color) :
                                             fallback_color;

  if (node.type == NodeType::Group)
  {
    if (node_json.contains("children") && node_json["children"].is_array())
    {
      for (const auto& child_json : node_json["children"])
      {
        if (child_json.is_object())
        {
          node.children.push_back(annotationNodeFromJson(child_json, node.color.isValid() ? node.color :
                                                                                           fallback_color));
        }
      }
    }
    return node;
  }

  node.annotation = annotationItemFromJson(node_json, node.color.isValid() ? node.color : fallback_color);
  if (node.name.isEmpty())
  {
    node.name = node.annotation.label;
  }
  return node;
}

json annotationNodeToJson(const AnnotationNode& node, const QColor& fallback_color)
{
  json node_json;
  node_json["type"] = (node.type == NodeType::Group) ? "group" : "annotation";
  if (!node.name.isEmpty())
  {
    node_json["name"] = node.name.toStdString();
  }
  node_json["visible"] = node.visible;
  if (node.color.isValid())
  {
    node_json["color"] = colorToJson(node.color);
  }

  if (node.type == NodeType::Group)
  {
    json children = json::array();
    for (const auto& child : node.children)
    {
      children.push_back(annotationNodeToJson(child, node.color.isValid() ? node.color : fallback_color));
    }
    node_json["children"] = children;
    return node_json;
  }

  const auto item_json = annotationItemToJson(node.annotation, fallback_color);
  for (auto it = item_json.begin(); it != item_json.end(); ++it)
  {
    node_json[it.key()] = it.value();
  }
  return node_json;
}

bool nodeUsesTreeStructure(const AnnotationNode& node)
{
  if (node.type == NodeType::Group)
  {
    return true;
  }
  if (!node.children.isEmpty())
  {
    return true;
  }
  return false;
}

void appendFlattenedItems(const QVector<AnnotationNode>& nodes,
                          QVector<AnnotationManager::AnnotationItem>& flat_items,
                          bool ancestors_visible, const QColor& inherited_color)
{
  for (const auto& node : nodes)
  {
    const QColor node_color = node.color.isValid() ? node.color : inherited_color;
    const bool effective_visible = ancestors_visible && node.visible;
    if (node.type == NodeType::Group)
    {
      appendFlattenedItems(node.children, flat_items, effective_visible, node_color);
      continue;
    }

    auto item = node.annotation;
    if (node_color.isValid())
    {
      item.color = node_color;
    }
    item.enabled = item.enabled && effective_visible;
    flat_items.push_back(item);
  }
}

void setNodeSubtreeEnabled(QVector<AnnotationNode>& nodes, bool enabled)
{
  for (auto& node : nodes)
  {
    node.visible = enabled;
    if (node.type == NodeType::Group)
    {
      setNodeSubtreeEnabled(node.children, enabled);
    }
    else
    {
      node.annotation.enabled = enabled;
    }
  }
}

AnnotationNode* nodeAtPath(QVector<AnnotationNode>& nodes, const QVector<int>& path)
{
  AnnotationNode* current = nullptr;
  QVector<AnnotationNode>* current_nodes = &nodes;
  for (const int index : path)
  {
    if (index < 0 || index >= current_nodes->size())
    {
      return nullptr;
    }
    current = &(*current_nodes)[index];
    current_nodes = &current->children;
  }
  return current;
}

const AnnotationNode* nodeAtPath(const QVector<AnnotationNode>& nodes, const QVector<int>& path)
{
  const AnnotationNode* current = nullptr;
  const QVector<AnnotationNode>* current_nodes = &nodes;
  for (const int index : path)
  {
    if (index < 0 || index >= current_nodes->size())
    {
      return nullptr;
    }
    current = &(*current_nodes)[index];
    current_nodes = &current->children;
  }
  return current;
}

bool removeNodeAtPath(QVector<AnnotationNode>& nodes, const QVector<int>& path)
{
  if (path.isEmpty())
  {
    return false;
  }

  if (path.size() == 1)
  {
    const int index = path.front();
    if (index < 0 || index >= nodes.size())
    {
      return false;
    }
    nodes.removeAt(index);
    return true;
  }

  QVector<int> parent_path = path;
  const int index = parent_path.takeLast();
  auto* parent = nodeAtPath(nodes, parent_path);
  if (!parent || index < 0 || index >= parent->children.size())
  {
    return false;
  }
  parent->children.removeAt(index);
  return true;
}

int flatIndexForPath(const QVector<AnnotationNode>& nodes, const QVector<int>& target_path)
{
  int flat_index = 0;
  bool found = false;
  QVector<int> path;
  std::function<void(const QVector<AnnotationNode>&)> visit = [&](const QVector<AnnotationNode>& current_nodes) {
    for (int i = 0; i < current_nodes.size() && !found; ++i)
    {
      path.push_back(i);
      const auto& node = current_nodes[i];
      if (node.type == NodeType::Group)
      {
        visit(node.children);
      }
      else
      {
        if (path == target_path)
        {
          found = true;
        }
        else
        {
          ++flat_index;
        }
      }
      path.pop_back();
    }
  };
  visit(nodes);
  return found ? flat_index : -1;
}
}  // namespace

AnnotationManager::AnnotationManager(QObject* parent) : QObject(parent)
{
}

const QVector<AnnotationManager::AnnotationLayer>& AnnotationManager::layers() const
{
  return _layers;
}

int AnnotationManager::activeLayerIndex() const
{
  return _active_layer_index;
}

AnnotationManager::AnnotationLayer* AnnotationManager::activeLayer()
{
  if (_active_layer_index < 0 || _active_layer_index >= _layers.size())
  {
    return nullptr;
  }
  return &_layers[_active_layer_index];
}

const AnnotationManager::AnnotationLayer* AnnotationManager::activeLayer() const
{
  if (_active_layer_index < 0 || _active_layer_index >= _layers.size())
  {
    return nullptr;
  }
  return &_layers[_active_layer_index];
}

int AnnotationManager::createLayer(const QString& name)
{
  AnnotationLayer layer;
  layer.name = name;
  layer.color = defaultLayerColor(_layers.size());
  _layers.push_back(layer);
  _active_layer_index = _layers.size() - 1;
  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
  return _active_layer_index;
}

bool AnnotationManager::loadLayer(const QString& file_path)
{
  AnnotationLayer layer;
  if (!loadLayerFromFile(file_path, layer))
  {
    return false;
  }

  _layers.push_back(layer);
  _active_layer_index = _layers.size() - 1;
  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
  return true;
}

bool AnnotationManager::saveLayer(int index)
{
  if (index < 0 || index >= _layers.size() || _layers[index].file_path.isEmpty())
  {
    return false;
  }
  if (!saveLayerToFile(_layers[index], _layers[index].file_path))
  {
    return false;
  }
  markLayerDirty(index, false);
  return true;
}

bool AnnotationManager::saveLayerAs(int index, const QString& file_path)
{
  if (index < 0 || index >= _layers.size())
  {
    return false;
  }
  _layers[index].file_path = file_path;
  if (_layers[index].name.isEmpty())
  {
    _layers[index].name = QFileInfo(file_path).completeBaseName();
  }
  if (!saveLayerToFile(_layers[index], file_path))
  {
    return false;
  }
  markLayerDirty(index, false);
  emit layersChanged();
  return true;
}

int AnnotationManager::duplicateLayer(int index, const QString& new_name)
{
  if (index < 0 || index >= _layers.size())
  {
    return -1;
  }

  const AnnotationLayer& source_layer = _layers[index];
  AnnotationLayer layer = source_layer;
  layer.file_path.clear();
  layer.dirty = true;
  layer.name = new_name.trimmed().isEmpty() ? (layer.name + " copy") : new_name.trimmed();
  layer.origin.kind = "duplicate";
  layer.origin.source_name = source_layer.name;
  layer.origin.source_file = source_layer.file_path;
  layer.origin.copied_at = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

  _layers.push_back(layer);
  _active_layer_index = _layers.size() - 1;
  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
  return _active_layer_index;
}

void AnnotationManager::removeLayer(int index)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }

  _layers.removeAt(index);
  if (_layers.isEmpty())
  {
    _active_layer_index = -1;
  }
  else if (_active_layer_index >= _layers.size())
  {
    _active_layer_index = _layers.size() - 1;
  }
  else if (_active_layer_index == index)
  {
    _active_layer_index = std::min(index, _layers.size() - 1);
  }

  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
}

void AnnotationManager::clear()
{
  if (_layers.isEmpty() && _active_layer_index == -1)
  {
    return;
  }

  _layers.clear();
  _active_layer_index = -1;
  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
}

void AnnotationManager::setActiveLayerIndex(int index)
{
  if (index < -1 || index >= _layers.size() || _active_layer_index == index)
  {
    return;
  }
  _active_layer_index = index;
  emit activeLayerChanged(_active_layer_index);
}

void AnnotationManager::setLayerVisible(int index, bool visible)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  if (_layers[index].visible == visible)
  {
    return;
  }
  _layers[index].visible = visible;
  emit layersChanged();
}

void AnnotationManager::setLayerEditable(int index, bool editable)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  if (_layers[index].editable == editable)
  {
    return;
  }
  _layers[index].editable = editable;
  emit layersChanged();
}

void AnnotationManager::setLayerName(int index, const QString& name)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  _layers[index].name = name;
  markLayerDirty(index);
}

void AnnotationManager::setLayerColor(int index, const QColor& color)
{
  if (index < 0 || index >= _layers.size() || !color.isValid())
  {
    return;
  }
  if (_layers[index].color == color)
  {
    return;
  }
  _layers[index].color = color;
  markLayerDirty(index);
  emit layersChanged();
}

void AnnotationManager::setLayerAxisId(int index, const QString& axis_id, bool explicit_axis)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  if (_layers[index].axis_id == axis_id && _layers[index].axis_explicit == explicit_axis)
  {
    return;
  }
  _layers[index].axis_id = axis_id;
  _layers[index].axis_kind = axisKindFromId(axis_id);
  _layers[index].axis_label = axis_id;
  _layers[index].axis_explicit = explicit_axis;
  markLayerDirty(index);
}

void AnnotationManager::setLayerItemsEnabled(int index, bool enabled)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }

  setNodeSubtreeEnabled(_layers[index].nodes, enabled);
  rebuildLayerItems(index);
  markLayerDirty(index);
  emit itemsChanged();
}

bool AnnotationManager::addItemToActiveLayer(const AnnotationItem& item)
{
  return addItemToLayer(_active_layer_index, item);
}

void AnnotationManager::updateActiveLayerItem(int row, const AnnotationItem& item)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable || row < 0 || row >= layer->items.size())
  {
    return;
  }
  int target_index = 0;
  std::function<bool(QVector<AnnotationNode>&)> visit = [&](QVector<AnnotationNode>& nodes) {
    for (auto& node : nodes)
    {
      if (node.type == NodeType::Group)
      {
        if (visit(node.children))
        {
          return true;
        }
        continue;
      }
      if (target_index == row)
      {
        node.annotation = item;
        node.name = item.label;
        return true;
      }
      ++target_index;
    }
    return false;
  };

  if (visit(layer->nodes))
  {
    rebuildLayerItems(_active_layer_index);
    markLayerDirty(_active_layer_index);
    emit itemsChanged();
  }
}

void AnnotationManager::removeActiveLayerItem(int row)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable || row < 0 || row >= layer->items.size())
  {
    return;
  }
  int target_index = 0;
  QVector<int> target_path;
  QVector<int> path;
  std::function<bool(const QVector<AnnotationNode>&)> find = [&](const QVector<AnnotationNode>& nodes) {
    for (int i = 0; i < nodes.size(); ++i)
    {
      path.push_back(i);
      const auto& node = nodes[i];
      if (node.type == NodeType::Group)
      {
        if (find(node.children))
        {
          return true;
        }
      }
      else
      {
        if (target_index == row)
        {
          target_path = path;
          return true;
        }
        ++target_index;
      }
      path.pop_back();
    }
    return false;
  };
  if (find(layer->nodes) && removeNodeAtPath(layer->nodes, target_path))
  {
    rebuildLayerItems(_active_layer_index);
    markLayerDirty(_active_layer_index);
    emit itemsChanged();
  }
}

bool AnnotationManager::addItemToLayer(int layer_index, const AnnotationItem& item,
                                       const QString& parent_node_path)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return false;
  }
  auto& layer = _layers[layer_index];
  if (!layer.editable)
  {
    return false;
  }

  AnnotationItem new_item = item;
  if (!new_item.color.isValid())
  {
    new_item.color = layer.color;
  }
  auto node = annotationNodeFromItem(new_item);

  if (parent_node_path.isEmpty())
  {
    layer.nodes.push_back(node);
  }
  else if (auto* parent = ::nodeAtPath(layer.nodes, pathFromString(parent_node_path));
           parent && parent->type == NodeType::Group)
  {
    parent->children.push_back(node);
  }
  else
  {
    return false;
  }

  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

bool AnnotationManager::addGroupToLayer(int layer_index, const QString& name,
                                        const QString& parent_node_path)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return false;
  }
  auto& layer = _layers[layer_index];
  if (!layer.editable)
  {
    return false;
  }

  AnnotationNode node;
  node.type = NodeType::Group;
  node.name = name.trimmed().isEmpty() ? QString("Group") : name.trimmed();
  node.visible = true;
  node.color = layer.color;

  if (parent_node_path.isEmpty())
  {
    layer.nodes.push_back(node);
  }
  else if (auto* parent = ::nodeAtPath(layer.nodes, pathFromString(parent_node_path));
           parent && parent->type == NodeType::Group)
  {
    parent->children.push_back(node);
  }
  else
  {
    return false;
  }

  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

bool AnnotationManager::updateLayerNodeAnnotation(int layer_index, const QString& node_path,
                                                  const AnnotationItem& item)
{
  auto* node = nodeAtPath(layer_index, node_path);
  if (!node || node->type != NodeType::Annotation)
  {
    return false;
  }
  node->annotation = item;
  node->name = item.label;
  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

bool AnnotationManager::setLayerNodeName(int layer_index, const QString& node_path, const QString& name)
{
  auto* node = nodeAtPath(layer_index, node_path);
  if (!node)
  {
    return false;
  }
  node->name = name.trimmed();
  if (node->type == NodeType::Annotation)
  {
    node->annotation.label = node->name;
  }
  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

bool AnnotationManager::setLayerNodeColor(int layer_index, const QString& node_path, const QColor& color)
{
  if (!color.isValid())
  {
    return false;
  }
  auto* node = nodeAtPath(layer_index, node_path);
  if (!node)
  {
    return false;
  }
  node->color = color;
  if (node->type == NodeType::Annotation)
  {
    node->annotation.color = color;
  }
  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

bool AnnotationManager::removeLayerNode(int layer_index, const QString& node_path)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return false;
  }
  if (!removeNodeAtPath(_layers[layer_index].nodes, pathFromString(node_path)))
  {
    return false;
  }
  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

std::optional<AnnotationNode> AnnotationManager::copyLayerNode(int layer_index,
                                                               const QString& node_path) const
{
  const auto* node = this->nodeAtPath(layer_index, node_path);
  if (!node)
  {
    return std::nullopt;
  }
  return *node;
}

bool AnnotationManager::insertLayerNode(int layer_index, const QString& parent_node_path,
                                        const AnnotationLayer::AnnotationNode& node, int child_index)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return false;
  }
  auto& layer = _layers[layer_index];
  if (!layer.editable)
  {
    return false;
  }

  auto insert_at = [&](QVector<AnnotationNode>& nodes) {
    if (child_index < 0 || child_index > nodes.size())
    {
      nodes.push_back(node);
    }
    else
    {
      nodes.insert(child_index, node);
    }
  };

  if (parent_node_path.isEmpty())
  {
    insert_at(layer.nodes);
  }
  else if (auto* parent = ::nodeAtPath(layer.nodes, pathFromString(parent_node_path));
           parent && parent->type == NodeType::Group)
  {
    insert_at(parent->children);
  }
  else
  {
    return false;
  }

  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
  return true;
}

void AnnotationManager::setLayerNodes(int layer_index, const QVector<AnnotationLayer::AnnotationNode>& nodes)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return;
  }
  _layers[layer_index].nodes = nodes;
  rebuildLayerItems(layer_index);
  markLayerDirty(layer_index);
  emit itemsChanged();
}

int AnnotationManager::flatItemIndexForNodePath(int layer_index, const QString& node_path) const
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return -1;
  }
  return flatIndexForPath(_layers[layer_index].nodes, pathFromString(node_path));
}

QString AnnotationManager::parentGroupPath(const QString& node_path) const
{
  auto path = pathFromString(node_path);
  if (path.isEmpty())
  {
    return {};
  }
  path.removeLast();
  return pathToString(path);
}

const AnnotationNode* AnnotationManager::nodeAtPath(int layer_index, const QString& node_path) const
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return nullptr;
  }
  return ::nodeAtPath(_layers[layer_index].nodes, pathFromString(node_path));
}

AnnotationNode* AnnotationManager::nodeAtPath(int layer_index, const QString& node_path)
{
  if (layer_index < 0 || layer_index >= _layers.size())
  {
    return nullptr;
  }
  return ::nodeAtPath(_layers[layer_index].nodes, pathFromString(node_path));
}

void AnnotationManager::rebuildLayerItems(int index)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  rebuildLayerItems(_layers[index]);
  emit itemsChanged();
}

QString AnnotationManager::pathToString(const QVector<int>& path)
{
  QStringList parts;
  for (const int index : path)
  {
    parts.push_back(QString::number(index));
  }
  return parts.join('/');
}

QVector<int> AnnotationManager::pathFromString(const QString& path)
{
  QVector<int> indices;
  if (path.isEmpty())
  {
    return indices;
  }
  const auto parts = path.split('/', Qt::SkipEmptyParts);
  for (const auto& part : parts)
  {
    bool ok = false;
    const int index = part.toInt(&ok);
    if (!ok)
    {
      return {};
    }
    indices.push_back(index);
  }
  return indices;
}

bool AnnotationManager::loadLayerFromFile(const QString& file_path, AnnotationLayer& layer) const
{
  QFile file(file_path);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    return false;
  }

  const auto data = file.readAll();
  json doc = json::parse(data.constData(), nullptr, false);
  if (doc.is_discarded() || !doc.is_object())
  {
    return false;
  }

  layer.name =
      QString::fromStdString(doc.value("name", QFileInfo(file_path).completeBaseName().toStdString()));
  layer.file_path = file_path;
  if (doc.contains("origin") && doc["origin"].is_object())
  {
    const auto& origin = doc["origin"];
    layer.origin.kind = QString::fromStdString(origin.value("kind", std::string()));
    layer.origin.source_name =
        QString::fromStdString(origin.value("source_name", std::string()));
    layer.origin.source_file =
        QString::fromStdString(origin.value("source_file", std::string()));
    layer.origin.copied_at =
        QString::fromStdString(origin.value("copied_at", std::string()));
  }
  if (doc.contains("axis") && doc["axis"].is_object())
  {
    const auto& axis = doc["axis"];
    layer.axis_id = QString::fromStdString(axis.value("id", std::string()));
    layer.axis_kind = QString::fromStdString(axis.value("kind", axisKindFromId(layer.axis_id).toStdString()));
    layer.axis_label =
        QString::fromStdString(axis.value("label", layer.axis_id.toStdString()));
    layer.axis_explicit = !layer.axis_id.isEmpty();
  }
  else if (doc.contains("axis_id"))
  {
    layer.axis_id = QString::fromStdString(doc.value("axis_id", std::string()));
    layer.axis_kind = axisKindFromId(layer.axis_id);
    layer.axis_label = layer.axis_id;
    layer.axis_explicit = !layer.axis_id.isEmpty();
  }
  else if (doc.contains("time_domain"))
  {
    const QString legacy_time_domain =
        QString::fromStdString(doc.value("time_domain", std::string("plot_time")));
    if (!legacy_time_domain.isEmpty() && legacy_time_domain != "plot_time")
    {
      layer.axis_id = legacy_time_domain;
      layer.axis_kind = axisKindFromId(layer.axis_id);
      layer.axis_label = layer.axis_id;
      layer.axis_explicit = true;
    }
  }
  else
  {
    layer.axis_id.clear();
    layer.axis_kind = "unknown";
    layer.axis_label.clear();
    layer.axis_explicit = false;
  }
  layer.visible = doc.value("visible", true);
  layer.editable = doc.value("editable", true);
  layer.dirty = false;
  if (doc.contains("color"))
  {
    layer.color = colorFromJson(doc["color"], Qt::yellow);
  }

  if (doc.contains("nodes") && doc["nodes"].is_array())
  {
    for (const auto& node_json : doc["nodes"])
    {
      if (node_json.is_object())
      {
        layer.nodes.push_back(annotationNodeFromJson(node_json, layer.color));
      }
    }
  }
  else if (doc.contains("items") && doc["items"].is_array())
  {
    for (const auto& item_json : doc["items"])
    {
      if (!item_json.is_object())
      {
        continue;
      }
      layer.nodes.push_back(annotationNodeFromItem(annotationItemFromJson(item_json, layer.color)));
    }
  }
  rebuildLayerItems(layer);
  return true;
}

bool AnnotationManager::saveLayerToFile(const AnnotationLayer& layer, const QString& file_path) const
{
  json doc;
  doc["schema"] = "plotjuggler.annotations";
  doc["version"] = 3;
  doc["name"] = layer.name.toStdString();
  if (!layer.origin.kind.isEmpty() || !layer.origin.source_name.isEmpty() ||
      !layer.origin.source_file.isEmpty() || !layer.origin.copied_at.isEmpty())
  {
    json origin;
    if (!layer.origin.kind.isEmpty())
    {
      origin["kind"] = layer.origin.kind.toStdString();
    }
    if (!layer.origin.source_name.isEmpty())
    {
      origin["source_name"] = layer.origin.source_name.toStdString();
    }
    if (!layer.origin.source_file.isEmpty())
    {
      origin["source_file"] = layer.origin.source_file.toStdString();
    }
    if (!layer.origin.copied_at.isEmpty())
    {
      origin["copied_at"] = layer.origin.copied_at.toStdString();
    }
    doc["origin"] = origin;
  }
  if (!layer.axis_id.isEmpty())
  {
    json axis;
    axis["id"] = layer.axis_id.toStdString();
    axis["kind"] = layer.axis_kind.toStdString();
    axis["label"] = (layer.axis_label.isEmpty() ? layer.axis_id : layer.axis_label).toStdString();
    doc["axis"] = axis;
  }
  doc["visible"] = layer.visible;
  doc["editable"] = layer.editable;
  doc["color"] = colorToJson(layer.color);

  bool use_tree_structure = false;
  for (const auto& node : layer.nodes)
  {
    if (nodeUsesTreeStructure(node))
    {
      use_tree_structure = true;
      break;
    }
  }

  if (use_tree_structure)
  {
    json nodes = json::array();
    for (const auto& node : layer.nodes)
    {
      nodes.push_back(annotationNodeToJson(node, layer.color));
    }
    doc["nodes"] = nodes;
  }
  else
  {
    json items = json::array();
    for (const auto& node : layer.nodes)
    {
      if (node.type != NodeType::Annotation)
      {
        continue;
      }
      items.push_back(annotationItemToJson(node.annotation, layer.color));
    }
    doc["items"] = items;
  }

  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    return false;
  }

  QTextStream stream(&file);
  stream << QString::fromStdString(doc.dump(2));
  return stream.status() == QTextStream::Ok;
}

void AnnotationManager::markLayerDirty(int index, bool dirty)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  if (_layers[index].dirty == dirty)
  {
    return;
  }
  _layers[index].dirty = dirty;
  emit layersChanged();
}

void AnnotationManager::rebuildLayerItems(AnnotationLayer& layer) const
{
  layer.items.clear();
  appendFlattenedItems(layer.nodes, layer.items, true, layer.color);
}
