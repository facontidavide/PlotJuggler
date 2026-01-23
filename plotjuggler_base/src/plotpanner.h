#ifndef PLOTPANNER_H
#define PLOTPANNER_H

#include "qwt_plot_panner.h"

class PlotPanner : public QwtPlotPanner
{
  Q_OBJECT

public:
  explicit PlotPanner(QWidget* canvas) : QwtPlotPanner(canvas)
  {
  }

public Q_SLOTS:
  void moveCanvas(int dx, int dy) override;

signals:
  void rescaled(QRectF new_size);

protected:
  void widgetMousePressEvent(QMouseEvent* event) override;
  void widgetMouseReleaseEvent(QMouseEvent* event) override;

private:
  bool _cursor_overridden = false;
};

#endif  // PLOTPANNER_H
