#ifndef DragableWidget_H
#define DragableWidget_H

#include <map>
#include <deque>
#include <QObject>
#include <QTextEdit>
#include <QDomDocument>
#include <QMessageBox>
#include <QTime>
#include "plotmagnifier.h"
#include "plotzoomer.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_symbol.h"
#include "qwt_legend.h"
#include "qwt_plot_rescaler.h"
#include "qwt_plot_panner.h"
#include "qwt_plot_legenditem.h"
#include "timeseries_qwt.h"
#include "customtracker.h"
#include "transforms/transform_selector.h"
#include "transforms/custom_function.h"
#include "plotlegend.h"

class PlotWidget : public QwtPlot
{
  Q_OBJECT

public:

  struct CurveInfo
  {
    std::string src_name;
    QwtPlotCurve* curve;
    QwtPlotMarker* marker;
  };

  PlotWidget(PlotDataMapRef& datamap, QWidget* parent = nullptr);

  void setContextMenuEnabled(bool enabled);

  virtual ~PlotWidget() override;

  bool isEmpty() const;

  const std::list<CurveInfo> &curveList() const;

  std::list<CurveInfo> &curveList();

  CurveInfo* curveFromTitle(const QString &title);

  const CurveInfo* curveFromTitle(const QString &title) const;

  QDomElement xmlSaveState(QDomDocument& doc) const;

  bool xmlLoadState(QDomElement& element);

  Range getMaximumRangeX() const;

  Range getMaximumRangeY(Range range_X) const;

  void setZoomRectangle(QRectF rect, bool emit_signal);

  void reloadPlotData();

  bool isXYPlot() const;

  void changeBackgroundColor(QColor color);

  const PlotLegend* legend() const
  {
    return _legend;
  }

  CurveInfo* addCurve(const std::string& name, QColor color = Qt::transparent);

  void setLegendSize(int size);

  bool isLegendVisible() const;

  void setLegendAlignment(Qt::Alignment alignment);

  void setZoomEnabled(bool enabled);

  bool isZoomEnabled() const;

  QRectF canvasBoundingRect() const;

  virtual void resizeEvent(QResizeEvent* ev) override;

  virtual void updateLayout() override;

  void setConstantRatioXY(bool active);

  PlotDataMapRef& datamap()
  {
    return _mapped_data;
  }

  double timeOffset() const
  {
    return _time_offset;
  }

  void changeCurveStyle(QwtPlotCurve::CurveStyle style);

  QwtPlotCurve::CurveStyle curveStyle() const
  {
    return _curve_style;
  }

  std::map<QString, QColor> getCurveColors() const;

  void setCustomAxisLimits(Range range);

  Range customAxisLimit() const;

  void removeCurve(const QString& title);

protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;

  bool canvasEventFilter(QEvent* event);

  QColor getColorHint(PlotData* data);


signals:
  void swapWidgetsRequested(PlotWidget* source, PlotWidget* destination);
  void rectChanged(PlotWidget* self, QRectF rect);
  void undoableChange();
  void trackerMoved(QPointF pos);
  void curveListChanged();
  void curvesDropped();
  void legendSizeChanged(int new_size);
  void splitHorizontal();
  void splitVertical();

public slots:

  void replot() override;

  void updateCurves();

  void onSourceDataRemoved(const std::string &src_name);

  void removeAllCurves();

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

  void on_changeCurveColor(const QString &curve_name, QColor new_color);

private slots:

  //void on_changeToBuiltinTransforms(QString new_transform);

  void setModeXY(bool enable);

  void on_savePlotToFile();

  void on_copyToClipboard();

  void on_copyAction_triggered();

  void on_pasteAction_triggered();

private slots:

  void canvasContextMenuTriggered(const QPoint& pos);

  void on_showPoints_triggered();

  void on_externallyResized(const QRectF& new_rect);

private:
  std::list<CurveInfo> _curve_list;

  QAction* _action_removeAllCurves;
  QAction* _action_edit;
  QAction* _action_formula;
  QAction* _action_split_horizontal;
  QAction* _action_split_vertical;

  QAction* _action_zoomOutMaximum;
  QAction* _action_zoomOutHorizontally;
  QAction* _action_zoomOutVertically;
  QAction* _action_saveToFile;
  QAction* _action_copy;
  QAction* _action_paste;
  QAction* _action_image_to_clipboard;

  PlotZoomer* _zoomer;
  PlotMagnifier* _magnifier;
  QwtPlotPanner* _panner1;
  QwtPlotPanner* _panner2;

  CurveTracker* _tracker;
  PlotLegend* _legend;
  QwtPlotGrid* _grid;

  bool _use_date_time_scale;

  int _color_index;
  static int global_color_index;

  PlotDataMapRef& _mapped_data;

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

  CurveInfo* addCurveXY(std::string name_x, std::string name_y, QString curve_name = "");

  void buildActions();

  void updateAvailableTransformers();

  QwtPlotCurve::CurveStyle _curve_style;

  void setDefaultRangeX();

  QwtSeriesWrapper* createCurveXY(const PlotData* data_x, const PlotData* data_y);

  QwtSeriesWrapper* createTimeSeries(const QString& transform_ID, const PlotData* data);

  double _time_offset;

  bool _xy_mode;

  Range _custom_Y_limits;

  TransformSelector* _transform_select_dialog;

  SnippetsMap _snippets;

  bool _zoom_enabled;

  bool _keep_aspect_ratio;

  QRectF _max_zoom_rect;

  bool _context_menu_enabled;

  void transformCustomCurves();
  void updateMaximumZoomArea();
  void rescaleEqualAxisScaling();
  void overrideCursonMove();
};

#endif
