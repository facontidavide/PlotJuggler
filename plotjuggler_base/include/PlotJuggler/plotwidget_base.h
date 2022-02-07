#ifndef PLOTWIDGET_BASE_H
#define PLOTWIDGET_BASE_H

#include <QWidget>
#include "plotdata.h"
#include "timeseries_qwt.h"

class QwtPlot;
class QwtPlotCurve;
class QwtPlotPanner;
class QwtPlotMarker;

class PlotZoomer;
class PlotMagnifier;
class PlotLegend;

namespace PJ
{
class PlotWidgetBase : public QObject
{
  Q_OBJECT

public:
  enum CurveStyle
  {
    LINES,
    DOTS,
    LINES_AND_DOTS,
    STICKS
  };

  struct CurveInfo
  {
    std::string src_name;
    QwtPlotCurve* curve;
    QwtPlotMarker* marker;
  };

  PlotWidgetBase(QWidget* parent);

  virtual ~PlotWidgetBase();

  virtual CurveInfo* addCurve(const std::string& name,
                              PlotDataXY &src_data,
                              QColor color = Qt::transparent);

  virtual void removeCurve(const QString& title);

  const std::list<CurveInfo>& curveList() const;
  std::list<CurveInfo>& curveList();

  bool isEmpty() const;

  QColor getColorHint(PlotDataXY *data);

  std::map<QString, QColor> getCurveColors() const;

  CurveInfo* curveFromTitle(const QString& title);

  virtual QwtSeriesWrapper* createTimeSeries(const QString& transform_ID,
                                             const PlotData* data);

  virtual void resetZoom();

  virtual PJ::Range getVisualizationRangeX() const;

  virtual PJ::Range getVisualizationRangeY(PJ::Range range_X) const;

  virtual void setModeXY(bool enable);

  void setLegendSize(int size);

  void setLegendAlignment(Qt::Alignment alignment);

  void setZoomEnabled(bool enabled);

  bool isZoomEnabled() const;

  void changeCurvesStyle(CurveStyle style);

  bool isXYPlot() const;

  QRectF canvasBoundingRect() const;

  QRectF maxZoomRect() const;

  QWidget* widget();

  const QWidget* widget() const;

  CurveStyle curveStyle() const;

  bool keepRatioXY() const;

  void setKeepRatioXY(bool active);

  void setAcceptDrops(bool accept);

public slots:

  void replot();

  virtual void removeAllCurves();

signals:

  void curveListChanged();

  void viewResized(const QRectF&);

  void dragEnterSignal(QDragEnterEvent* event);

  void dropSignal(QDropEvent* event);

  void legendSizeChanged(int new_size);

  void widgetResized();

protected:
  class QwtPlotPimpl;
  QwtPlotPimpl* p = nullptr;

  static void setStyle(QwtPlotCurve* curve, CurveStyle style);

  QwtPlot* qwtPlot();
  const QwtPlot* qwtPlot() const;

  PlotLegend* legend();
  PlotZoomer* zoomer();
  PlotMagnifier* magnifier();

  void updateMaximumZoomArea();

  bool eventFilter(QObject* obj, QEvent* event);

private:
  bool _xy_mode;

  QRectF _max_zoom_rect;

  bool _keep_aspect_ratio;
};

}  // namespace PJ

#endif  // PLOTWIDGET_PROXY_H
