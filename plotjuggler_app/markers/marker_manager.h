#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QVector>

class MarkerManager : public QObject
{
  Q_OBJECT

public:
  enum class MarkerType
  {
    Point,
    Region
  };

  struct MarkerItem
  {
    bool enabled = true;
    MarkerType type = MarkerType::Point;
    double start_time = 0.0;
    double end_time = 0.0;
    QString label;
    QString tags;
    QString notes;
    QColor color = Qt::yellow;
  };

  struct MarkerLayer
  {
    struct OriginInfo
    {
      QString kind;
      QString source_name;
      QString source_file;
      QString copied_at;
    };

    QString name;
    QString file_path;
    QString axis_id;
    QString axis_kind = "unknown";
    QString axis_label;
    bool axis_explicit = false;
    bool visible = true;
    bool editable = true;
    bool dirty = false;
    QColor color = Qt::yellow;
    OriginInfo origin;
    QVector<MarkerItem> items;
  };

  explicit MarkerManager(QObject* parent = nullptr);

  const QVector<MarkerLayer>& layers() const;
  int activeLayerIndex() const;
  MarkerLayer* activeLayer();
  const MarkerLayer* activeLayer() const;

  int createLayer(const QString& name);
  bool loadLayer(const QString& file_path);
  bool saveLayer(int index);
  bool saveLayerAs(int index, const QString& file_path);
  int duplicateLayer(int index, const QString& new_name = QString());
  void removeLayer(int index);

  void setActiveLayerIndex(int index);
  void setLayerVisible(int index, bool visible);
  void setLayerEditable(int index, bool editable);
  void setLayerName(int index, const QString& name);
  void setLayerAxisId(int index, const QString& axis_id, bool explicit_axis = true);

  bool addItemToActiveLayer(const MarkerItem& item);
  void updateActiveLayerItem(int row, const MarkerItem& item);
  void removeActiveLayerItem(int row);

signals:
  void layersChanged();
  void activeLayerChanged(int index);
  void itemsChanged();

private:
  bool loadLayerFromFile(const QString& file_path, MarkerLayer& layer) const;
  bool saveLayerToFile(const MarkerLayer& layer, const QString& file_path) const;
  void markLayerDirty(int index, bool dirty = true);

  QVector<MarkerLayer> _layers;
  int _active_layer_index = -1;
};
