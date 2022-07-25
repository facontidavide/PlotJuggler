/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PLOT_BACKGROUND_H
#define PLOT_BACKGROUND_H

#include <QBrush>
#include "qwt_plot_zoneitem.h"

#include "PlotJuggler/plotdata.h"
#include "color_map.h"

class BackgroundColorItem : public QwtPlotItem
{
public:
  BackgroundColorItem(const PJ::PlotData& data, QString colormap_name);

  virtual ~BackgroundColorItem() override = default;

  void setTimeOffset(double* time_offset)
  {
    _time_offset = time_offset;
  }

  int rtti() const override
  {
    return QwtPlotItem::Rtti_PlotZone;
  }

  virtual void draw(QPainter* painter, const QwtScaleMap&, const QwtScaleMap&,
                    const QRectF& canvasRect) const override;

  virtual QRectF boundingRect() const override;

  QString colormapName() const
  {
    return _colormap_name;
  }

  QString dataName() const
  {
    return _data_name;
  }

private:
  const PJ::PlotData& _data;
  QString _data_name;
  QString _colormap_name;
  double* _time_offset = nullptr;
};

#endif  // PLOT_BACKGROUND_H
