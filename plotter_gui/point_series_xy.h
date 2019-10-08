#ifndef POINT_SERIES_H
#define POINT_SERIES_H

#include "series_data.h"

class PointSeriesXY: public DataSeriesBase
{
public:
    PointSeriesXY(const PlotData* y_axis, const PlotData* x_axis);

    virtual QPointF sample( size_t i ) const override
    {
        const auto& p = _cached_curve.at(i);
        return QPointF(p.x, p.y);
    }

    QRectF boundingRect() const override
    {
        return _bounding_box;
    }

    PlotData::RangeTimeOpt getVisualizationRangeX() override;

    nonstd::optional<QPointF> sampleFromTime(double t) override;

    PlotData::RangeValueOpt getVisualizationRangeY(PlotData::RangeTime range_X) override;

    bool updateCache() override;

    const PlotData* dataX() const { return _x_axis; }
    const PlotData* dataY() const { return _y_axis; }
    void setOffset(double offsetX, double offsetY);

protected:

    const PlotData *_x_axis;
    const PlotData *_y_axis;
    double _offsetX;
    double _offsetY;
    PlotData _cached_curve;
};

#endif // POINT_SERIES_H
