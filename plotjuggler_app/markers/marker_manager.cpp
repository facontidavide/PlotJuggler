#include "marker_manager.h"

#include "nlohmann/json.hpp"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <array>
#include <algorithm>

namespace
{
using json = nlohmann::json;

QColor defaultLayerColor(int index)
{
  static const std::array<QColor, 6> palette = {
    QColor("#0ea5a4"), QColor("#f59e0b"), QColor("#ef4444"),
    QColor("#3b82f6"), QColor("#8b5cf6"), QColor("#84cc16")
  };
  return palette[static_cast<size_t>(index) % palette.size()];
}

QString markerTypeToString(MarkerManager::MarkerType type)
{
  return (type == MarkerManager::MarkerType::Point) ? "point" : "region";
}

MarkerManager::MarkerType markerTypeFromString(const QString& type)
{
  return (type.compare("region", Qt::CaseInsensitive) == 0) ? MarkerManager::MarkerType::Region :
                                                               MarkerManager::MarkerType::Point;
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
}  // namespace

MarkerManager::MarkerManager(QObject* parent) : QObject(parent)
{
}

const QVector<MarkerManager::MarkerLayer>& MarkerManager::layers() const
{
  return _layers;
}

int MarkerManager::activeLayerIndex() const
{
  return _active_layer_index;
}

MarkerManager::MarkerLayer* MarkerManager::activeLayer()
{
  if (_active_layer_index < 0 || _active_layer_index >= _layers.size())
  {
    return nullptr;
  }
  return &_layers[_active_layer_index];
}

const MarkerManager::MarkerLayer* MarkerManager::activeLayer() const
{
  if (_active_layer_index < 0 || _active_layer_index >= _layers.size())
  {
    return nullptr;
  }
  return &_layers[_active_layer_index];
}

int MarkerManager::createLayer(const QString& name)
{
  MarkerLayer layer;
  layer.name = name;
  layer.color = defaultLayerColor(_layers.size());
  _layers.push_back(layer);
  _active_layer_index = _layers.size() - 1;
  emit layersChanged();
  emit activeLayerChanged(_active_layer_index);
  return _active_layer_index;
}

bool MarkerManager::loadLayer(const QString& file_path)
{
  MarkerLayer layer;
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

bool MarkerManager::saveLayer(int index)
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

bool MarkerManager::saveLayerAs(int index, const QString& file_path)
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

int MarkerManager::duplicateLayer(int index, const QString& new_name)
{
  if (index < 0 || index >= _layers.size())
  {
    return -1;
  }

  const MarkerLayer& source_layer = _layers[index];
  MarkerLayer layer = source_layer;
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

void MarkerManager::removeLayer(int index)
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

void MarkerManager::setActiveLayerIndex(int index)
{
  if (index < -1 || index >= _layers.size() || _active_layer_index == index)
  {
    return;
  }
  _active_layer_index = index;
  emit activeLayerChanged(_active_layer_index);
}

void MarkerManager::setLayerVisible(int index, bool visible)
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

void MarkerManager::setLayerEditable(int index, bool editable)
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

void MarkerManager::setLayerName(int index, const QString& name)
{
  if (index < 0 || index >= _layers.size())
  {
    return;
  }
  _layers[index].name = name;
  markLayerDirty(index);
}

void MarkerManager::setLayerAxisId(int index, const QString& axis_id, bool explicit_axis)
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

bool MarkerManager::addItemToActiveLayer(const MarkerItem& item)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable)
  {
    return false;
  }
  MarkerItem new_item = item;
  if (!new_item.color.isValid())
  {
    new_item.color = layer->color;
  }
  layer->items.push_back(new_item);
  markLayerDirty(_active_layer_index);
  emit itemsChanged();
  return true;
}

void MarkerManager::updateActiveLayerItem(int row, const MarkerItem& item)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable || row < 0 || row >= layer->items.size())
  {
    return;
  }
  layer->items[row] = item;
  markLayerDirty(_active_layer_index);
  emit itemsChanged();
}

void MarkerManager::removeActiveLayerItem(int row)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable || row < 0 || row >= layer->items.size())
  {
    return;
  }
  layer->items.removeAt(row);
  markLayerDirty(_active_layer_index);
  emit itemsChanged();
}

bool MarkerManager::loadLayerFromFile(const QString& file_path, MarkerLayer& layer) const
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

  if (doc.contains("items") && doc["items"].is_array())
  {
    for (const auto& item_json : doc["items"])
    {
      if (!item_json.is_object())
      {
        continue;
      }
      MarkerItem item;
      item.enabled = item_json.value("enabled", true);
      item.type =
          markerTypeFromString(QString::fromStdString(item_json.value("type", std::string("point"))));
      item.start_time = item_json.value("start", item_json.value("start_time", 0.0));
      item.end_time = item_json.value("end", item_json.value("end_time", item.start_time));
      item.label = QString::fromStdString(item_json.value("label", std::string()));
      item.tags = QString::fromStdString(item_json.value("tags", std::string()));
      item.notes = QString::fromStdString(item_json.value("notes", std::string()));
      if (item_json.contains("color"))
      {
        item.color = colorFromJson(item_json["color"], layer.color);
      }
      else
      {
        item.color = layer.color;
      }
      layer.items.push_back(item);
    }
  }
  return true;
}

bool MarkerManager::saveLayerToFile(const MarkerLayer& layer, const QString& file_path) const
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

  json items = json::array();
  for (const auto& item : layer.items)
  {
    json item_json;
    item_json["enabled"] = item.enabled;
    item_json["type"] = markerTypeToString(item.type).toStdString();
    item_json["start"] = item.start_time;
    item_json["end"] = item.end_time;
    item_json["label"] = item.label.toStdString();
    item_json["tags"] = item.tags.toStdString();
    item_json["notes"] = item.notes.toStdString();
    item_json["color"] = colorToJson(item.color);
    items.push_back(item_json);
  }
  doc["items"] = items;

  QFile file(file_path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    return false;
  }

  QTextStream stream(&file);
  stream << QString::fromStdString(doc.dump(2));
  return stream.status() == QTextStream::Ok;
}

void MarkerManager::markLayerDirty(int index, bool dirty)
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
