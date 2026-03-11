#pragma once

#include <QWidget>

#include "marker_manager.h"

namespace Ui
{
class MarkersPanel;
}

class QListWidgetItem;

class MarkersPanel : public QWidget
{
  Q_OBJECT

public:
  explicit MarkersPanel(MarkerManager* manager, QWidget* parent = nullptr);
  ~MarkersPanel() override;

  void setCurrentTime(double time);
  void setCurrentViewRange(double start_time, double end_time);
  bool hasSelectedMarker() const;
  MarkerManager::MarkerItem selectedMarker() const;
  int selectedMarkerRow() const;
  bool isActiveLayerEditable() const;
  void updateSelectedMarker(const MarkerManager::MarkerItem& item);

private slots:
  void refreshLayers();
  void refreshItems();
  void onLayerSelectionChanged();
  void onLayerItemChanged(QListWidgetItem* item);
  void onTableCellChanged(int row, int column);
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

signals:
  void selectedMarkerChanged(bool has_selection);
  void jumpToSelectedMarkerRequested();

private:
  void updateButtons();
  void populateLayerList();
  void populateItemTable();
  MarkerManager::MarkerItem markerItemFromRow(int row) const;
  void setTableRow(int row, const MarkerManager::MarkerItem& item);

  Ui::MarkersPanel* ui = nullptr;
  MarkerManager* _manager = nullptr;
  double _current_time = 0.0;
  double _view_start_time = 0.0;
  double _view_end_time = 1.0;
  bool _updating_ui = false;
};
