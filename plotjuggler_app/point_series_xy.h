#ifndef POINT_SERIES_H
#define POINT_SERIES_H

#include "timeseries_qwt.h"

class PointSeriesXY : public QwtTimeseries
{
public:
  PointSeriesXY(const PlotData* x_axis, const PlotData* y_axis);

  virtual QPointF sample(size_t i) const override
  {
    const auto& p = _cached_curve.at(i);
    return QPointF(p.x, p.y);
  }

  size_t size() const override;

  std::optional<QPointF> sampleFromTime(double t) override;

  RangeOpt getVisualizationRangeY(Range range_X) override;

  void updateCache(bool reset_old_data) override;

  RangeOpt getVisualizationRangeX() override;

  const PlotData* dataX() const
  {
    return _x_axis;
  }
  const PlotData* dataY() const
  {
    return _y_axis;
  }

  const PlotDataXY* plotData() const override
  {
    return &_cached_curve;
  }

protected:
  const PlotData* _x_axis;
  const PlotData* _y_axis;
  PlotDataXY _cached_curve;
};

#endif  // POINT_SERIES_H
