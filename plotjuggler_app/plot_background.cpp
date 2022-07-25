/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "plot_background.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"

BackgroundColorItem::BackgroundColorItem(const PJ::PlotData& data, QString colormap_name)
  : _data(data)
  , _data_name(QString::fromStdString(data.plotName()))
  , _colormap_name(colormap_name)
{
}

void BackgroundColorItem::draw(QPainter* painter, const QwtScaleMap& xMap,
                               const QwtScaleMap& /*yMap*/,
                               const QRectF& canvasRect) const
{
  if (_data.size() < 2)
  {
    return;
  }
  auto it = ColorMapLibrary().find(_colormap_name);
  if (it == ColorMapLibrary().end())
  {
    return;
  }
  auto colormap = it->second;

  double prev_Y = _data[0].y;
  double min_interval = _data[0].x;
  QColor prev_color = colormap->mapColor(prev_Y);

  auto isEqual = [](double a, double b) {
    return abs(a - b) < std::numeric_limits<float>::epsilon();
  };

  const size_t N = _data.size();
  double time_offset = _time_offset ? (*_time_offset) : 0;

  for (size_t i = 1; i < N; i++)
  {
    const auto& point = _data[i];
    if (isEqual(prev_Y, point.y) && (i + 1) < N)
    {
      continue;
    }
    QColor color = colormap->mapColor(point.y);
    if (prev_color != color || (i + 1) == N)
    {
      double max_interval = point.x;
      double x1 = xMap.transform(min_interval - time_offset);
      double x2 = xMap.transform(max_interval - time_offset);

      QRectF r(x1, canvasRect.top(), x2 - x1, canvasRect.height());
      r = r.normalized();

      if (x1 < x2 && prev_color != Qt::transparent)
      {
        QwtPainter::fillRect(painter, r, prev_color);
      }
      prev_color = color;
      min_interval = max_interval;
    }
    prev_Y = point.y;
  }
}

QRectF BackgroundColorItem::boundingRect() const
{
  QRectF br = QwtPlotItem::boundingRect();
  auto range_x = _data.rangeX();
  if (range_x)
  {
    br.setLeft(_data.rangeX()->min);
    br.setRight(_data.rangeX()->max);
  }
  return br;
}
