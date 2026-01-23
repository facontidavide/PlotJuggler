#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "qwt_scale_div.h"

#include "plotpanner.h"
#include <QMouseEvent>
#include <QApplication>
#include <QSettings>
#include "PlotJuggler/svg_util.h"

void PlotPanner::moveCanvas(int dx, int dy)
{
  if (dx == 0 && dy == 0)
    return;

  QwtPlot* plot = this->plot();
  if (plot == NULL)
    return;

  const bool doAutoReplot = plot->autoReplot();
  plot->setAutoReplot(false);

  QRectF new_rect;

  for (int axisPos = 0; axisPos < QwtAxis::AxisPositions; axisPos++)
  {
    const QwtAxisId axisId(axisPos);

    if (!isAxisEnabled(axisId))
      continue;

    const QwtScaleMap map = plot->canvasMap(axisId);

    const double p1 = map.transform(plot->axisScaleDiv(axisId).lowerBound());
    const double p2 = map.transform(plot->axisScaleDiv(axisId).upperBound());

    double d1, d2;
    if (QwtAxis::isXAxis(axisPos))
    {
      d1 = map.invTransform(p1 - dx);
      d2 = map.invTransform(p2 - dx);
    }
    else
    {
      d1 = map.invTransform(p1 - dy);
      d2 = map.invTransform(p2 - dy);
    }

    plot->setAxisScale(axisId, d1, d2);

    if (axisId == QwtPlot::yLeft)
    {
      new_rect.setBottom(d1);
      new_rect.setTop(d2);
    }
    if (axisId == QwtPlot::xBottom)
    {
      new_rect.setLeft(d1);
      new_rect.setRight(d2);
    }
  }

  emit rescaled(new_rect);

  plot->setAutoReplot(doAutoReplot);
  plot->replot();
}

void PlotPanner::widgetMousePressEvent(QMouseEvent* event)
{
  // Check if this event matches our panning button/modifiers
  Qt::MouseButton button;
  Qt::KeyboardModifiers modifiers;
  getMouseButton(button, modifiers);

  if (event->button() == button && event->modifiers() == modifiers)
  {
    // Set the move cursor when panning starts
    QSettings settings;
    QString theme = settings.value("Preferences::theme", "light").toString();
    auto pixmap = LoadSvg(":/resources/svg/move_view.svg", theme);
    QApplication::setOverrideCursor(QCursor(pixmap.scaled(24, 24)));
    _cursor_overridden = true;
  }

  QwtPlotPanner::widgetMousePressEvent(event);
}

void PlotPanner::widgetMouseReleaseEvent(QMouseEvent* event)
{
  // Restore cursor before calling base class
  if (_cursor_overridden)
  {
    QApplication::restoreOverrideCursor();
    _cursor_overridden = false;
  }

  QwtPlotPanner::widgetMouseReleaseEvent(event);
}
