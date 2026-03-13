#pragma once

#include <QDir>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QWidget>

#include "annotation_manager.h"

namespace Ui
{
class AnnotationsPanel;
}

class QTreeWidgetItem;
class QMenu;

class AnnotationsPanel : public QWidget
{
  Q_OBJECT

public:
  explicit AnnotationsPanel(AnnotationManager* manager, QWidget* parent = nullptr);
  ~AnnotationsPanel() override;

  void setCurrentTime(double time);
  void setCurrentViewRange(double start_time, double end_time);
  void setStreamingActive(bool active);
  bool hasSelectedAnnotation() const;
  AnnotationManager::AnnotationItem selectedAnnotation() const;
  int selectedAnnotationRow() const;
  bool isActiveLayerEditable() const;
  void updateSelectedAnnotation(const AnnotationManager::AnnotationItem& item);
  void setSessionDataFiles(const QStringList& file_paths);
  void setCurrentAxisId(const QString& axis_id);
  void setSessionTimeRange(double start_time, double end_time);
  void setAutoloadCompanionAnnotations(bool enabled);
  bool autoloadCompanionAnnotations() const;
  void clearForSessionChange();
  bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
  void refreshAnnotationLayers();
  void refreshAnnotationItems();
  void onTreeCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
  void onTreeItemChanged(QTreeWidgetItem* item, int column);
  void onNewLayer();
  void onLoadLayer();
  void onSaveLayer();
  void onSaveLayerAs();
  void onDuplicateLayer();
  void onRemoveLayer();
  void onRenameLayer();
  void onEditableToggled(bool checked);
  void onAddPoint();
  void onAddRegion();
  void onRemoveItem();
  void onJumpToItem();
  void onTreeContextMenu(const QPoint& pos);
  void onEditLayerProperties();
  void onAnnotationEnabledEdited(bool checked);
  void onAnnotationLabelEdited();
  void onAnnotationTagsEdited();
  void onAnnotationNotesEdited();
  void flushPendingAnnotationTextEdits();

signals:
  void selectedAnnotationChanged(bool has_selection);
  void jumpToSelectedAnnotationRequested();
  void autoloadCompanionAnnotationsChanged(bool enabled);
  void generateAnnotationsRequested(int layer_index, const QString& node_path);

private:
  void scheduleTreeRefresh();
  void updateButtons();
  void populateTree();
  void syncTreeStructureToManager();
  QList<QTreeWidgetItem*> selectedTreeItems() const;
  QList<QTreeWidgetItem*> selectedTreeItemsForNodeOps() const;
  QList<QTreeWidgetItem*> selectedTreeItemsForClipboard() const;
  void selectTreeNodeLater(int layer_index, const QString& node_path);
  QTreeWidgetItem* findTreeItem(int layer_index, const QString& node_path) const;
  std::optional<AnnotationManager::AnnotationLayer::AnnotationNode> buildNodeFromTreeItem(
      const QTreeWidgetItem* item) const;
  QString exportNodesAsTsv(const QList<QTreeWidgetItem*>& items) const;
  bool importNodesFromTsv(const QString& tsv_text);
  void copyCurrentNodeToClipboard();
  void cutCurrentNodeToClipboard();
  void pasteNodeFromClipboard();
  void updateDetailsPanel();
  void openLayerMenu(int layer_index, const QPoint& global_pos);
  void openGroupMenu(int layer_index, const QString& node_path, const QPoint& global_pos);
  void openAnnotationMenu(int layer_index, int annotation_row, const QString& node_path,
                          const QPoint& global_pos);
  bool isLayerCompatible(const AnnotationManager::AnnotationLayer& layer) const;
  bool isCurrentSelectionCompatible() const;
  bool isAnnotationInSessionRange(const AnnotationManager::AnnotationItem& item) const;
  void addLayerItem(int layer_index, const AnnotationManager::AnnotationLayer& layer);
  void addNodeItem(QTreeWidgetItem* parent_item, int layer_index,
                   const AnnotationManager::AnnotationLayer& layer,
                   const AnnotationManager::AnnotationLayer::AnnotationNode& node,
                   const QVector<int>& node_path, int& enabled_leaf_count, int& total_leaf_count);
  void installTreeShortcuts();
  QTreeWidgetItem* currentTreeItem() const;
  int layerIndexFromItem(const QTreeWidgetItem* item) const;
  int annotationRowFromItem(const QTreeWidgetItem* item) const;
  QString nodePathFromItem(const QTreeWidgetItem* item) const;
  QString defaultAnnotationDirectory() const;
  QString defaultAnnotationStem() const;
  QString suggestGroupName(int layer_index, const QString& parent_node_path) const;
  bool editGroupPropertiesDialog(int layer_index, const QString& node_path);
  QString suggestDuplicateLayerName(const QString& source_name) const;
  QString sanitizeAnnotationVariant(QString text) const;
  QString autoloadSafeAnnotationBaseName(const QString& layer_name) const;
  QString suggestUniqueLayerName() const;
  QString nextCopyLayerName(const QString& base_name, const QSet<QString>& used_names) const;
  QString nextCopyFileBaseName(const QString& base_name, const QDir& dir) const;
  QString suggestUniqueAnnotationFilePath() const;
  bool editLayerPropertiesDialog(int layer_index);

  Ui::AnnotationsPanel* ui = nullptr;
  AnnotationManager* _manager = nullptr;
  QStringList _session_data_files;
  QString _current_axis_id;
  double _session_start_time = 0.0;
  double _session_end_time = 1.0;
  bool _session_time_range_valid = false;
  double _current_time = 0.0;
  double _view_start_time = 0.0;
  double _view_end_time = 1.0;
  bool _streaming_active = false;
  bool _updating_ui = false;
  bool _tree_refresh_pending = false;
  bool _tree_structure_sync_pending = false;
  int _detail_layer_index = -1;
  int _detail_annotation_row = -1;
  QTimer* _annotation_text_edit_timer = nullptr;
};
