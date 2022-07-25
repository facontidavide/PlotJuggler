/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <limits>
#include <QDebug>
#include <QWheelEvent>
#include <QApplication>

#include "plotmagnifier.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"

PlotMagnifier::PlotMagnifier(QWidget* canvas)
  : QwtPlotMagnifier(canvas), _default_mode(BOTH_AXES)
{
  for (int axisId = 0; axisId < QwtPlot::axisCnt; axisId++)
  {
    _lower_bounds[axisId] = std::numeric_limits<double>::lowest();
    _upper_bounds[axisId] = std::numeric_limits<double>::max();
  }
}

PlotMagnifier::~PlotMagnifier()
{
}

void PlotMagnifier::setAxisLimits(int axis, double lower, double upper)
{
  if (axis >= 0 && axis < QwtPlot::axisCnt)
  {
    _lower_bounds[axis] = lower;
    _upper_bounds[axis] = upper;
  }
}

void PlotMagnifier::rescale(double factor, AxisMode axis)
{
  factor = qAbs(1.0 / factor);

  QwtPlot* plt = plot();
  if (plt == nullptr || factor == 1.0)
  {
    return;
  }

  bool doReplot = false;

  const bool autoReplot = plt->autoReplot();
  plt->setAutoReplot(false);

  const int axis_list[2] = { QwtPlot::xBottom, QwtPlot::yLeft };
  QRectF new_rect;

  for (int i = 0; i < 2; i++)
  {
    double temp_factor = factor;
    if (i == 1 && axis == X_AXIS)
    {
      temp_factor = 1.0;
    }
    if (i == 0 && axis == Y_AXIS)
    {
      temp_factor = 1.0;
    }

    int axisId = axis_list[i];

    if (isAxisEnabled(axisId))
    {
      const QwtScaleMap scaleMap = plt->canvasMap(axisId);

      double v1 = scaleMap.s1();
      double v2 = scaleMap.s2();
      double center = _mouse_position.x();

      if (axisId == QwtPlot::yLeft)
      {
        center = _mouse_position.y();
      }

      if (scaleMap.transformation())
      {
        // the coordinate system of the paint device is always linear
        v1 = scaleMap.transform(v1);  // scaleMap.p1()
        v2 = scaleMap.transform(v2);  // scaleMap.p2()
      }

      const double width = (v2 - v1);
      const double ratio = (v2 - center) / (width);

      v1 = center - width * temp_factor * (1 - ratio);
      v2 = center + width * temp_factor * (ratio);

      bool reversed_axis = false;
      if (v1 > v2)
      {
        reversed_axis = true;
        std::swap(v1, v2);
      }

      if (scaleMap.transformation())
      {
        v1 = scaleMap.invTransform(v1);
        v2 = scaleMap.invTransform(v2);
      }

      v1 = std::max(v1, _lower_bounds[axisId]);
      v2 = std::min(v2, _upper_bounds[axisId]);

      if (reversed_axis)
      {
        plt->setAxisScale(axisId, v2, v1);
      }
      else
      {
        plt->setAxisScale(axisId, v1, v2);
      }

      if (axisId == QwtPlot::xBottom)
      {
        new_rect.setLeft(v1);
        new_rect.setRight(v2);
      }
      else
      {
        new_rect.setBottom(v1);
        new_rect.setTop(v2);
      }

      doReplot = true;
    }
  }

  plt->setAutoReplot(autoReplot);

  if (doReplot)
  {
    emit rescaled(new_rect);
  }
}

QPointF PlotMagnifier::invTransform(QPoint pos)
{
  QwtScaleMap xMap = plot()->canvasMap(QwtPlot::xBottom);
  QwtScaleMap yMap = plot()->canvasMap(QwtPlot::yLeft);
  return QPointF(xMap.invTransform(pos.x()), yMap.invTransform(pos.y()));
}

void PlotMagnifier::widgetWheelEvent(QWheelEvent* event)
{
  _mouse_position = invTransform(event->pos());
  QwtPlotMagnifier::widgetWheelEvent(event);
}

void PlotMagnifier::widgetMousePressEvent(QMouseEvent* event)
{
  _mouse_position = invTransform(event->pos());
  QwtPlotMagnifier::widgetMousePressEvent(event);
}
