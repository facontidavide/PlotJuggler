#pragma once

#include <QDir>
#include <QSet>
#include <QStringList>
#include <QWidget>

#include "annotation_manager.h"

namespace Ui
{
class AnnotationsPanel;
}

class QTreeWidgetItem;

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
  void setAutoloadCompanionAnnotations(bool enabled);
  bool autoloadCompanionAnnotations() const;
  void clearForSessionChange();

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
  void onAddRegion();
  void onRemoveItem();
  void onJumpToItem();

signals:
  void selectedAnnotationChanged(bool has_selection);
  void jumpToSelectedAnnotationRequested();
  void autoloadCompanionAnnotationsChanged(bool enabled);

private:
  void scheduleTreeRefresh();
  void updateButtons();
  void populateTree();
  bool isLayerCompatible(const AnnotationManager::AnnotationLayer& layer) const;
  bool isCurrentSelectionCompatible() const;
  AnnotationManager::AnnotationItem annotationItemFromTree(QTreeWidgetItem* item) const;
  void addLayerItem(int layer_index, const AnnotationManager::AnnotationLayer& layer);
  QTreeWidgetItem* currentTreeItem() const;
  int layerIndexFromItem(const QTreeWidgetItem* item) const;
  int annotationRowFromItem(const QTreeWidgetItem* item) const;
  QString defaultAnnotationDirectory() const;
  QString defaultAnnotationStem() const;
  QString suggestDuplicateLayerName(const QString& source_name) const;
  QString sanitizeAnnotationVariant(QString text) const;
  QString autoloadSafeAnnotationBaseName(const QString& layer_name) const;
  QString suggestUniqueLayerName() const;
  QString nextCopyLayerName(const QString& base_name, const QSet<QString>& used_names) const;
  QString nextCopyFileBaseName(const QString& base_name, const QDir& dir) const;
  QString suggestUniqueAnnotationFilePath() const;

  Ui::AnnotationsPanel* ui = nullptr;
  AnnotationManager* _manager = nullptr;
  QStringList _session_data_files;
  QString _current_axis_id;
  double _current_time = 0.0;
  double _view_start_time = 0.0;
  double _view_end_time = 1.0;
  bool _streaming_active = false;
  bool _updating_ui = false;
  bool _tree_refresh_pending = false;
};
