#include "plotzoomer.h"
#include <QMouseEvent>
#include <QApplication>
#include <QSettings>
#include <QPen>
#include "qwt_scale_map.h"
#include "qwt_plot.h"

PlotZoomer::PlotZoomer(QWidget* canvas)
  : QwtPlotZoomer(canvas, true), _mouse_pressed(false), _zoom_enabled(false), _keep_aspect_ratio(true)
{
  this->setTrackerMode(AlwaysOff);
}

void PlotZoomer::widgetMousePressEvent(QMouseEvent* me)
{
  _mouse_pressed = false;
  auto patterns = this->mousePattern();
  for (QwtEventPattern::MousePattern& pattern : patterns)
  {
    if (this->mouseMatch(pattern, me))
    {
      _mouse_pressed = true;
      // this->setTrackerMode(AlwaysOn);
      _initial_pos = me->pos();
    }
    break;
  }
  QwtPlotPicker::widgetMousePressEvent(me);
}

void PlotZoomer::widgetMouseMoveEvent(QMouseEvent* me)
{
  if (_mouse_pressed)
  {
    auto patterns = this->mousePattern();
    for (QwtEventPattern::MousePattern& pattern : patterns)
    {
      QRect rect(me->pos(), _initial_pos);
      QRectF zoomRect = invTransform(rect.normalized());

      if (zoomRect.width() > minZoomSize().width() && zoomRect.height() > minZoomSize().height())
      {
        if (!_zoom_enabled)
        {
          QSettings settings;
          QString theme = settings.value("Preferences::theme", "light").toString();
          QPixmap pixmap(tr(":/style_%1/zoom_in.png").arg(theme));
          QCursor zoom_cursor(pixmap.scaled(24, 24));

          _zoom_enabled = true;
          this->setRubberBand(RectRubberBand);
          this->setTrackerMode(AlwaysOff);
          QPen pen(parentWidget()->palette().foreground().color(), 1, Qt::DashLine);
          this->setRubberBandPen(pen);
          QApplication::setOverrideCursor(zoom_cursor);
        }
      }
      else if (_zoom_enabled)
      {
        _zoom_enabled = false;
        this->setRubberBand(NoRubberBand);
        QApplication::restoreOverrideCursor();
      }
      break;
    }
  }
  QwtPlotPicker::widgetMouseMoveEvent(me);
}

void PlotZoomer::widgetMouseReleaseEvent(QMouseEvent* me)
{
  _mouse_pressed = false;
  _zoom_enabled = false;
  QwtPlotPicker::widgetMouseReleaseEvent(me);
  this->setTrackerMode(AlwaysOff);
}

bool PlotZoomer::accept(QPolygon& pa) const
{
  QApplication::restoreOverrideCursor();

  if (pa.count() < 2)
    return false;

  QRect rect = QRect(pa[0], pa[int(pa.count()) - 1]);
  QRectF zoomRect = invTransform(rect.normalized());

  if (zoomRect.width() < minZoomSize().width() && zoomRect.height() < minZoomSize().height())
  {
    return false;
  }
  return QwtPlotZoomer::accept(pa);
}

void PlotZoomer::zoom(const QRectF& zoomRect)
{
  QRectF rect = zoomRect;

  if (_keep_aspect_ratio)
  {
    const QRectF cr = canvas()->contentsRect();
    const double canvas_ratio = cr.width() / cr.height();
    const double zoom_ratio = zoomRect.width() / zoomRect.height();

    if (zoom_ratio < canvas_ratio)
    {
      double new_width = zoomRect.height() * canvas_ratio;
      double increment = new_width - zoomRect.width();
      rect.setWidth(new_width);
      rect.moveLeft(rect.left() - 0.5 * increment);
    }
    else
    {
      double new_height = zoomRect.width() / canvas_ratio;
      double increment = new_height - zoomRect.height();
      rect.setHeight(new_height);
      rect.moveTop(rect.top() - 0.5 * increment);
    }
  }
  QwtPlotZoomer::zoom(rect);
}

QSizeF PlotZoomer::minZoomSize() const
{
  return QSizeF(scaleRect().width() * 0.02, scaleRect().height() * 0.02);
}
