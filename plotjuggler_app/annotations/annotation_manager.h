#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QVector>

class AnnotationManager : public QObject
{
  Q_OBJECT

public:
  enum class AnnotationType
  {
    Point,
    Region
  };

  struct AnnotationItem
  {
    bool enabled = true;
    AnnotationType type = AnnotationType::Point;
    double start_time = 0.0;
    double end_time = 0.0;
    QString label;
    QString tags;
    QString notes;
    QColor color = Qt::yellow;
  };

  struct AnnotationLayer
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
    QVector<AnnotationItem> items;
  };

  explicit AnnotationManager(QObject* parent = nullptr);

  const QVector<AnnotationLayer>& layers() const;
  int activeLayerIndex() const;
  AnnotationLayer* activeLayer();
  const AnnotationLayer* activeLayer() const;

  int createLayer(const QString& name);
  bool loadLayer(const QString& file_path);
  bool saveLayer(int index);
  bool saveLayerAs(int index, const QString& file_path);
  int duplicateLayer(int index, const QString& new_name = QString());
  void removeLayer(int index);
  void clear();

  void setActiveLayerIndex(int index);
  void setLayerVisible(int index, bool visible);
  void setLayerEditable(int index, bool editable);
  void setLayerName(int index, const QString& name);
  void setLayerAxisId(int index, const QString& axis_id, bool explicit_axis = true);

  bool addItemToActiveLayer(const AnnotationItem& item);
  void updateActiveLayerItem(int row, const AnnotationItem& item);
  void removeActiveLayerItem(int row);

signals:
  void layersChanged();
  void activeLayerChanged(int index);
  void itemsChanged();

private:
  bool loadLayerFromFile(const QString& file_path, AnnotationLayer& layer) const;
  bool saveLayerToFile(const AnnotationLayer& layer, const QString& file_path) const;
  void markLayerDirty(int index, bool dirty = true);

  QVector<AnnotationLayer> _layers;
  int _active_layer_index = -1;
};
