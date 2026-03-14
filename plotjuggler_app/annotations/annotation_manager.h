#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

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
    QColor color;
  };

  struct AnnotationLayer
  {
    enum class NodeType
    {
      Group,
      Annotation
    };

    struct OriginInfo
    {
      QString kind;
      QString source_name;
      QString source_file;
      QString copied_at;
    };

    struct AnnotationNode
    {
      NodeType type = NodeType::Annotation;
      QString name;
      bool visible = true;
      QColor color;
      AnnotationItem annotation;
      QVector<AnnotationNode> children;
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
    QVector<AnnotationNode> nodes;
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
  void setLayerColor(int index, const QColor& color);
  void setLayerAxisId(int index, const QString& axis_id, bool explicit_axis = true);
  void setLayerItemsEnabled(int index, bool enabled);

  bool addItemToActiveLayer(const AnnotationItem& item);
  void updateActiveLayerItem(int row, const AnnotationItem& item);
  void removeActiveLayerItem(int row);
  bool addItemToLayer(int layer_index, const AnnotationItem& item,
                      const QString& parent_node_path = QString());
  bool addGroupToLayer(int layer_index, const QString& name,
                       const QString& parent_node_path = QString());
  bool updateLayerNodeAnnotation(int layer_index, const QString& node_path,
                                 const AnnotationItem& item);
  bool setLayerNodeName(int layer_index, const QString& node_path, const QString& name);
  bool setLayerNodeColor(int layer_index, const QString& node_path, const QColor& color);
  bool removeLayerNode(int layer_index, const QString& node_path);
  std::optional<AnnotationLayer::AnnotationNode> copyLayerNode(int layer_index,
                                                               const QString& node_path) const;
  bool insertLayerNode(int layer_index, const QString& parent_node_path,
                       const AnnotationLayer::AnnotationNode& node, int child_index = -1);
  void setLayerNodes(int layer_index, const QVector<AnnotationLayer::AnnotationNode>& nodes);
  int flatItemIndexForNodePath(int layer_index, const QString& node_path) const;
  QString parentGroupPath(const QString& node_path) const;
  const AnnotationLayer::AnnotationNode* nodeAtPath(int layer_index,
                                                    const QString& node_path) const;
  AnnotationLayer::AnnotationNode* nodeAtPath(int layer_index, const QString& node_path);
  void rebuildLayerItems(int index);
  static QString pathToString(const QVector<int>& path);
  static QVector<int> pathFromString(const QString& path);

signals:
  void layersChanged();
  void activeLayerChanged(int index);
  void itemsChanged();

private:
  bool loadLayerFromFile(const QString& file_path, AnnotationLayer& layer) const;
  bool saveLayerToFile(const AnnotationLayer& layer, const QString& file_path) const;
  void markLayerDirty(int index, bool dirty = true);
  void rebuildLayerItems(AnnotationLayer& layer) const;

  QVector<AnnotationLayer> _layers;
  int _active_layer_index = -1;
};
