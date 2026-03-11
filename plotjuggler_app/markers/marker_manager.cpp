#include "marker_manager.h"

#include "nlohmann/json.hpp"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>

namespace
{
using json = nlohmann::json;

QString markerTypeToString(MarkerManager::MarkerType type)
{
  return (type == MarkerManager::MarkerType::Point) ? "point" : "region";
}

MarkerManager::MarkerType markerTypeFromString(const QString& type)
{
  return (type.compare("region", Qt::CaseInsensitive) == 0) ? MarkerManager::MarkerType::Region :
                                                               MarkerManager::MarkerType::Point;
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

  MarkerLayer layer = _layers[index];
  layer.file_path.clear();
  layer.dirty = true;
  layer.name = new_name.trimmed().isEmpty() ? (layer.name + " copy") : new_name.trimmed();

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

bool MarkerManager::addItemToActiveLayer(const MarkerItem& item)
{
  auto* layer = activeLayer();
  if (!layer || !layer->editable)
  {
    return false;
  }
  layer->items.push_back(item);
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
  layer.time_domain = QString::fromStdString(doc.value("time_domain", std::string("plot_time")));
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
      item.start_time = item_json.value("start_time", 0.0);
      item.end_time = item_json.value("end_time", item.start_time);
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
  doc["version"] = 1;
  doc["name"] = layer.name.toStdString();
  doc["time_domain"] = layer.time_domain.toStdString();
  doc["visible"] = layer.visible;
  doc["editable"] = layer.editable;
  doc["color"] = colorToJson(layer.color);

  json items = json::array();
  for (const auto& item : layer.items)
  {
    json item_json;
    item_json["enabled"] = item.enabled;
    item_json["type"] = markerTypeToString(item.type).toStdString();
    item_json["start_time"] = item.start_time;
    item_json["end_time"] = item.end_time;
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
