/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef DragableWidget_H
#define DragableWidget_H

#include <map>
#include <deque>
#include <QObject>
#include <QTextEdit>
#include <QDomDocument>
#include <QMessageBox>
#include <QTime>

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_symbol.h"
#include "qwt_legend.h"
#include "qwt_plot_rescaler.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_legenditem.h"

#include "PlotJuggler/plotwidget_base.h"
#include "customtracker.h"
#include "colormap_editor.h"

#include "transforms/transform_selector.h"
#include "transforms/custom_function.h"

#include "plot_background.h"

class StatisticsDialog;

class PlotWidget : public PlotWidgetBase
{
  Q_OBJECT

public:
  PlotWidget(PlotDataMapRef& datamap, QWidget* parent);

  void setContextMenuEnabled(bool enabled);

  virtual ~PlotWidget() override;

  QDomElement xmlSaveState(QDomDocument& doc) const;

  bool xmlLoadState(QDomElement& element, bool autozoom = true);

  Range getVisualizationRangeY(Range range_X) const override;

  void setZoomRectangle(QRectF rect, bool emit_signal);

  void reloadPlotData();

  double timeOffset() const
  {
    return _time_offset;
  }

  PlotDataMapRef& datamap()
  {
    return _mapped_data;
  }

  CurveInfo* addCurveXY(std::string name_x, std::string name_y, QString curve_name = "");

  CurveInfo* addCurve(const std::string& name, QColor color = Qt::transparent);

  void setCustomAxisLimits(Range range);

  Range customAxisLimit() const;

  void removeCurve(const QString& title) override;

  bool isZoomLinkEnabled() const;

  void setStatisticsTitle(QString title);

  void updateStatistics(bool forceUpdate = false);

protected:
  PlotDataMapRef& _mapped_data;

  bool eventFilter(QObject* obj, QEvent* event) override;
  void onDragEnterEvent(QDragEnterEvent* event);
  void onDragLeaveEvent(QDragLeaveEvent* event);
  void onDropEvent(QDropEvent* event);

  bool canvasEventFilter(QEvent* event);

signals:
  void swapWidgetsRequested(PlotWidget* source, PlotWidget* destination);
  void rectChanged(PlotWidget* self, QRectF rect);
  void undoableChange();
  void trackerMoved(QPointF pos);
  void curveListChanged();
  void curvesDropped();
  void splitHorizontal();
  void splitVertical();

public slots:

  void updateCurves(bool reset_older_data);

  void onDataSourceRemoved(const std::string& src_name);

  void removeAllCurves() override;

  void on_panned(int dx, int dy);

  void zoomOut(bool emit_signal);

  void on_zoomOutHorizontal_triggered(bool emit_signal = true);

  void on_zoomOutVertical_triggered(bool emit_signal = true);

  void activateLegend(bool activate);

  void activateGrid(bool activate);

  void configureTracker(CurveTracker::Parameter val);

  void enableTracker(bool enable);

  bool isTrackerEnabled() const;

  void setTrackerPosition(double abs_time);

  void on_changeTimeOffset(double offset);

  void on_changeDateTimeScale(bool enable);

  void on_changeCurveColor(const QString& curve_name, QColor new_color);

  void onFlipAxis();

  void onBackgroundColorRequest(QString name);

  void onShowDataStatistics();

private slots:

  // void on_changeToBuiltinTransforms(QString new_transform);

  void setModeXY(bool enable) override;

  void on_savePlotToFile();

  void on_copyToClipboard();

  void on_copyAction_triggered();

  void on_pasteAction_triggered();

private slots:

  void canvasContextMenuTriggered(const QPoint& pos);

  void on_externallyResized(const QRectF& new_rect);

private:
  QAction* _action_removeAllCurves;
  QAction* _action_edit;
  QAction* _action_formula;
  QAction* _action_split_horizontal;
  QAction* _action_split_vertical;
  QAction* _action_data_statistics;

  QAction* _action_zoomOutMaximum;
  QAction* _action_zoomOutHorizontally;
  QAction* _action_zoomOutVertically;
  QAction* _action_saveToFile;
  QAction* _action_copy;
  QAction* _action_paste;
  QAction* _action_image_to_clipboard;

  QAction* _flip_x;
  QAction* _flip_y;

  CurveTracker* _tracker;
  QwtPlotGrid* _grid;

  QString _statistics_window_title = "";

  std::unique_ptr<BackgroundColorItem> _background_item;

  bool _use_date_time_scale;

  StatisticsDialog* _statistics_dialog = nullptr;

  struct DragInfo
  {
    enum
    {
      NONE,
      CURVES,
      NEW_XY
    } mode;
    std::vector<QString> curves;
    QObject* source;
  };

  DragInfo _dragging;

  void buildActions();

  void updateAvailableTransformers();

  void setDefaultRangeX();

  QwtSeriesWrapper* createCurveXY(const PlotData* data_x, const PlotData* data_y);

  QwtSeriesWrapper* createTimeSeries(const PlotData* data,
                                     const QString& transform_ID = {}) override;

  double _time_offset;

  Range _custom_Y_limits;

  TransformSelector* _transform_select_dialog;

  SnippetsMap _snippets;

  bool _context_menu_enabled;

  // void updateMaximumZoomArea();
  void rescaleEqualAxisScaling();
  void overrideCursonMove();

  void setAxisScale(QwtAxisId axisId, double min, double max);
};

#endif
