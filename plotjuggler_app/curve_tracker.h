/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QEvent>
#include <QPointF>
#include <optional>
#include "qwt_plot_picker.h"
#include "qwt_plot_marker.h"

class QwtPlotCurve;

std::optional<QPointF> curvePointAt(const QwtPlotCurve* curve, double x);

class CurveTracker : public QObject
{
  Q_OBJECT
public:
  explicit CurveTracker(QwtPlot*, QColor color);

  ~CurveTracker();

  QPointF actualPosition() const;

  typedef enum
  {
    LINE_ONLY,
    VALUE,
    VALUE_NAME
  } Parameter;

public slots:

  void setPosition(const QPointF& pos);

  void setReferencePosition(std::optional<QPointF> reference_pos);

  void setParameter(Parameter par);

  void setEnabled(bool enable);

  bool isEnabled() const;

  void redraw()
  {
    setPosition(_prev_trackerpoint);
  }

private:
  QPointF transform(QPoint);

  QPoint invTransform(QPointF);

  QPointF _prev_trackerpoint;
  std::optional<QPointF> _reference_pos;
  std::vector<QwtPlotMarker*> _point_markers;
  QwtPlotMarker* _line_marker;
  QwtPlotMarker* _text_marker;
  QwtPlot* _plot;
  Parameter _param;
  bool _visible;
};
