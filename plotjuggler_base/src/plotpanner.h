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

  void widgetKeyPressEvent(QKeyEvent* ke) override;

signals:
  void rescaled(QRectF new_size);

private:
};

#endif  // PLOTPANNER_H
