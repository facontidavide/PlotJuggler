#pragma once

#include <QStringList>
#include <QWidget>

#include "marker_manager.h"

namespace Ui
{
class MarkersPanel;
}

class QTreeWidgetItem;

class MarkersPanel : public QWidget
{
  Q_OBJECT

public:
  explicit MarkersPanel(MarkerManager* manager, QWidget* parent = nullptr);
  ~MarkersPanel() override;

  void setCurrentTime(double time);
  void setCurrentViewRange(double start_time, double end_time);
  void setStreamingActive(bool active);
  bool hasSelectedMarker() const;
  MarkerManager::MarkerItem selectedMarker() const;
  int selectedMarkerRow() const;
  bool isActiveLayerEditable() const;
  void updateSelectedMarker(const MarkerManager::MarkerItem& item);
  void setSessionDataFiles(const QStringList& file_paths);
  void setCurrentAxisId(const QString& axis_id);
  void setAutoloadCompanionMarkup(bool enabled);
  bool autoloadCompanionMarkup() const;

private slots:
  void refreshLayers();
  void refreshItems();
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
  void selectedMarkerChanged(bool has_selection);
  void jumpToSelectedMarkerRequested();
  void autoloadCompanionMarkupChanged(bool enabled);

private:
  void scheduleTreeRefresh();
  void updateButtons();
  void populateTree();
  bool isLayerCompatible(const MarkerManager::MarkerLayer& layer) const;
  bool isCurrentSelectionCompatible() const;
  MarkerManager::MarkerItem markerItemFromTree(QTreeWidgetItem* item) const;
  void addLayerItem(int layer_index, const MarkerManager::MarkerLayer& layer);
  QTreeWidgetItem* currentTreeItem() const;
  int layerIndexFromItem(const QTreeWidgetItem* item) const;
  int markerRowFromItem(const QTreeWidgetItem* item) const;
  QString defaultMarkupDirectory() const;
  QString defaultMarkupStem() const;
  QString suggestDuplicateLayerName(const QString& source_name) const;
  QString sanitizeMarkupVariant(QString text) const;
  QString autoloadSafeMarkupBaseName(const QString& layer_name) const;
  QString suggestUniqueLayerName() const;
  QString suggestUniqueMarkupFilePath() const;

  Ui::MarkersPanel* ui = nullptr;
  MarkerManager* _manager = nullptr;
  QStringList _session_data_files;
  QString _current_axis_id;
  double _current_time = 0.0;
  double _view_start_time = 0.0;
  double _view_end_time = 1.0;
  bool _streaming_active = false;
  bool _updating_ui = false;
  bool _tree_refresh_pending = false;
};
