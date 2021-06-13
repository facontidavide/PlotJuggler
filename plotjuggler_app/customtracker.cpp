#include "customtracker.h"
#include "qwt_series_data.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_event_pattern.h"
#include "qwt_scale_map.h"
#include "qwt_symbol.h"
#include "qwt_graphic.h"
#include "qwt_text.h"
#include <qevent.h>
#include <QFontDatabase>

struct compareX
{
  inline bool operator()(const double x, const QPointF& pos) const
  {
    return (x < pos.x());
  }
};

CurveTracker::CurveTracker(QwtPlot* plot) : QObject(plot), _plot(plot), _param(VALUE)
{
  _line_marker = new QwtPlotMarker();

  _line_marker->setLinePen(QPen(Qt::red));
  _line_marker->setLineStyle(QwtPlotMarker::VLine);
  _line_marker->setValue(0, 0);
  _line_marker->attach(plot);

  _text_marker = new QwtPlotMarker();
  _text_marker->attach(plot);

  _visible = true;
}

CurveTracker::~CurveTracker()
{
}

QPointF CurveTracker::actualPosition() const
{
  return _prev_trackerpoint;
}

void CurveTracker::setParameter(Parameter par)
{
  bool changed = _param != par;
  _param = par;

  if (changed)
  {
    setPosition(_prev_trackerpoint);
  }
}

void CurveTracker::setEnabled(bool enable)
{
  _visible = enable;
  _line_marker->setVisible(enable);
  _text_marker->setVisible(enable);

  for (int i = 0; i < _marker.size(); i++)
  {
    _marker[i]->setVisible(enable);
  }
}

bool CurveTracker::isEnabled() const
{
  return _visible;
}

void CurveTracker::setPosition(const QPointF& position)
{
  const QwtPlotItemList curves = _plot->itemList(QwtPlotItem::Rtti_PlotCurve);

  _line_marker->setValue(position);

  QRectF rect;
  rect.setBottom(_plot->canvasMap(QwtPlot::yLeft).s1());
  rect.setTop(_plot->canvasMap(QwtPlot::yLeft).s2());
  rect.setLeft(_plot->canvasMap(QwtPlot::xBottom).s1());
  rect.setRight(_plot->canvasMap(QwtPlot::xBottom).s2());

  double min_Y = std::numeric_limits<double>::max();
  double max_Y = -min_Y;
  int visible_points = 0;

  while (_marker.size() > curves.size())
  {
    _marker.back()->detach();
    _marker.pop_back();
  }

  for (int i = _marker.size(); i < curves.size(); i++)
  {
    _marker.push_back(new QwtPlotMarker);
    _marker[i]->attach(_plot);
  }

  double text_X_offset = 0;

  std::multimap<double, QString> text_lines;

  for (int i = 0; i < curves.size(); i++)
  {
    QwtPlotCurve* curve = static_cast<QwtPlotCurve*>(curves[i]);
    _marker[i]->setVisible(curve->isVisible());

    if (curve->isVisible() == false)
    {
      continue;
    }
    QColor color = curve->pen().color();

    text_X_offset = rect.width() * 0.02;

    if (!_marker[i]->symbol() || _marker[i]->symbol()->brush().color() != color)
    {
      QwtSymbol* sym = new QwtSymbol(QwtSymbol::Ellipse, color, QPen(Qt::black), QSize(5, 5));
      _marker[i]->setSymbol(sym);
    }

    const QLineF line = curveLineAt(curve, position.x());

    if (line.isNull())
    {
      continue;
    }

    QPointF point;
    double middle_X = (line.p1().x() + line.p2().x()) / 2.0;

    if (position.x() < middle_X)
      point = line.p1();
    else
      point = line.p2();

    _marker[i]->setValue(point);

    if (rect.contains(point) && _visible)
    {
      min_Y = std::min(min_Y, point.y());
      max_Y = std::max(max_Y, point.y());

      visible_points++;
      double val = point.y();

      QString line;

      if (_param == VALUE)
      {
        line = QString("<font color=%1>%2</font>").arg(color.name()).arg(val);
      }
      else if (_param == VALUE_NAME)
      {
        QString value = QString::number(val, 'f', 3);
        int whitespaces = 8 - value.length();
        while (whitespaces-- > 0)
          value.prepend("&nbsp;");

        line = QString("<font color=%1>%2 : %3</font>").arg(color.name()).arg(value).arg(curve->title().text());
      }

      text_lines.insert(std::make_pair(val, line));
      _marker[i]->setVisible(true);
    }
    else
    {
      _marker[i]->setVisible(false);
    }
    _marker[i]->setValue(point);
  }

  QwtText mark_text;

  QString text_marker_info;

  int count = 0;
  for (auto it = text_lines.rbegin(); it != text_lines.rend(); it++)
  {
    text_marker_info += it->second;
    if (count++ < text_lines.size() - 1)
    {
      text_marker_info += "<br>";
    }
  }
  mark_text.setBorderPen(QColor(Qt::transparent));

  QColor background_color = _plot->palette().background().color();
  background_color.setAlpha(180);
  mark_text.setBackgroundBrush(background_color);
  mark_text.setText(text_marker_info);

  QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  font.setPointSize(9);

  mark_text.setFont(font);
  mark_text.setRenderFlags(_param == VALUE ? Qt::AlignCenter : Qt::AlignLeft);

  _text_marker->setLabel(mark_text);
  _text_marker->setLabelAlignment(Qt::AlignRight);

  _text_marker->setXValue(position.x() + text_X_offset);

  if (visible_points > 0)
  {
    _text_marker->setYValue(0.5 * (max_Y + min_Y));
  }

  double canvas_ratio = rect.width() / double(_plot->width());
  double text_width = mark_text.textSize().width() * canvas_ratio;
  bool exceed_right = (_text_marker->boundingRect().right() + text_width) > rect.right();

  if (exceed_right)
  {
    _text_marker->setXValue(position.x() - text_X_offset - text_width);
  }

  _text_marker->setVisible(visible_points > 0 && _visible && _param != LINE_ONLY);

  _prev_trackerpoint = position;
}

QLineF CurveTracker::curveLineAt(const QwtPlotCurve* curve, double x) const
{
  QLineF line;

  if (curve->dataSize() >= 2)
  {
    int index = qwtUpperSampleIndex<QPointF>(*curve->data(), x, compareX());

    if (index > 0)
    {
      line.setP1(curve->sample(index - 1));
      line.setP2(curve->sample(index));
    }
  }
  return line;
}
