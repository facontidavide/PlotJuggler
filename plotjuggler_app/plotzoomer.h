#ifndef PLOTZOOMER_H
#define PLOTZOOMER_H

#include <QObject>
#include <QPoint>
#include "qwt_plot_zoomer.h"

class PlotZoomer : public QwtPlotZoomer
{
public:
  PlotZoomer();

  explicit PlotZoomer(QWidget*);

  virtual ~PlotZoomer() override = default;

  void keepAspectRatio(bool doKeep)
  {
    _keep_aspect_ratio = doKeep;
  }

protected:
  virtual void widgetMousePressEvent(QMouseEvent* event) override;
  virtual void widgetMouseReleaseEvent(QMouseEvent* event) override;
  virtual void widgetMouseMoveEvent(QMouseEvent* event) override;
  virtual bool accept(QPolygon&) const override;

  virtual void zoom(const QRectF& rect) override;

  virtual QSizeF minZoomSize() const override;

private:
  bool _mouse_pressed;
  bool _zoom_enabled;
  bool _keep_aspect_ratio;
  QPoint _initial_pos;
};

#endif  // PLOTZOOMER_H
