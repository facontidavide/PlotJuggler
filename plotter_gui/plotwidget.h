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
#include "axis_limits_dialog.h"
#include "transforms/transform_selector.h"
#include "transforms/custom_function.h"
#include "plotlegend.h"

class PlotWidget : public QwtPlot
{
  Q_OBJECT

public:
  PlotWidget(PlotDataMapRef& datamap, QWidget* parent = nullptr);

  virtual ~PlotWidget() override;

  bool isEmpty() const;

  const std::map<std::string, QwtPlotCurve*>& curveList() const;

  QDomElement xmlSaveState(QDomDocument& doc) const;

  bool xmlLoadState(QDomElement& element);

  PlotData::RangeTime getMaximumRangeX() const;

  PlotData::RangeValue getMaximumRangeY(PlotData::RangeTime range_X) const;

  void setZoomRectangle(QRectF rect, bool emit_signal);

  void reloadPlotData();

  bool isXYPlot() const;

  void changeBackgroundColor(QColor color);

  const PlotLegend* legend() const
  {
    return _legend;
  }

  void setLegendSize(int size);

  bool isLegendVisible() const;

  void setLegendAlignment(Qt::Alignment alignment);

  void setZoomEnabled(bool enabled);

  bool isZoomEnabled() const;

  QRectF canvasBoundingRect() const;

  virtual void resizeEvent(QResizeEvent* ev) override;

  virtual void updateLayout() override;

  void setConstantRatioXY(bool active);

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

public slots:

  void replot() override;

  void updateCurves();

  void removeAllCurves();

  void on_panned(int dx, int dy);

  void zoomOut(bool emit_signal);

  void on_zoomOutHorizontal_triggered(bool emit_signal = true);

  void on_zoomOutVertical_triggered(bool emit_signal = true);

  void removeCurve(const std::string& name);

  void activateLegend(bool activate);

  void activateGrid(bool activate);

  void configureTracker(CurveTracker::Parameter val);

  void enableTracker(bool enable);

  bool isTrackerEnabled() const;

  void setTrackerPosition(double abs_time);

  void on_changeTimeOffset(double offset);

  void on_changeDateTimeScale(bool enable);

private slots:

  void on_changeToBuiltinTransforms(QString new_transform);

  void convertToXY();

  void on_customTransformsDialog();

  void on_savePlotToFile();

  void on_editAxisLimits_triggered();

  void on_copyToClipboard();

private slots:
  void launchRemoveCurveDialog();

  void canvasContextMenuTriggered(const QPoint& pos);

  void on_changeColorsDialog_triggered();

  void on_changeColor(QString curve_name, QColor new_color);

  void on_showPoints_triggered();

  void on_externallyResized(const QRectF& new_rect);

private:
  std::map<std::string, QwtPlotCurve*> _curve_list;
  std::map<std::string, QwtPlotMarker*> _point_marker;

  QAction* _action_removeCurve;
  QAction* _action_removeAllCurves;
  QAction* _action_changeColorsDialog;
  QAction* _action_showPoints;
  QAction* _action_zoomOutMaximum;
  QAction* _action_zoomOutHorizontally;
  QAction* _action_zoomOutVertically;
  QAction* _action_noTransform;
  QAction* _action_1stDerivativeTransform;
  QAction* _action_2ndDerivativeTransform;
  QAction* _action_custom_transform;
  QAction* _action_XY_transform;
  QAction* _action_saveToFile;
  QAction* _action_editLimits;
  QAction* _action_clipboard;

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
  QString _default_transform;
  std::map<std::string, QString> _curves_transform;

  struct DragInfo
  {
    enum
    {
      NONE,
      CURVES,
      NEW_XY,
      SWAP_PLOTS
    } mode;
    std::vector<QString> curves;
    QObject* source;
  };

  DragInfo _dragging;

  bool addCurve(const std::string& name);

  bool addCurveXY(std::string name_x, std::string name_y, QString curve_name = "");

  void buildActions();

  void updateAvailableTransformers();

  QwtPlotCurve::CurveStyle _curve_style;

  void setDefaultRangeX();

  DataSeriesBase* createCurveXY(const PlotData* data_x, const PlotData* data_y);

  DataSeriesBase* createTimeSeries(const QString& ID, const PlotData* data);

  double _time_offset;

  bool _xy_mode;

  PlotData::RangeValue _custom_Y_limits;

  AxisLimitsDialog* _axis_limits_dialog;

  TransformSelector* _transform_select_dialog;

  SnippetsMap _snippets;

  bool _zoom_enabled;

  bool _keep_aspect_ratio;

  QRectF _max_zoom_rect;

  void transformCustomCurves();
  void updateMaximumZoomArea();
  void rescaleEqualAxisScaling();
  void overrideCursonMove();
};

#endif
