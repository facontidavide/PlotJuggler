#ifndef PLOTDATA_QWT_H
#define PLOTDATA_QWT_H

#include <QColor>
#include <qwt_series_data.h>
#include <qwt_plot_marker.h>
#include "PlotJuggler/plotdata.h"



class TimeseriesQwt: public QwtSeriesData<QPointF>
{
public:

    TimeseriesQwt(PlotDataPtr base);

    virtual ~TimeseriesQwt() {}

    virtual QPointF sample( size_t i ) const override;

    virtual QRectF boundingRect() const override;

    virtual size_t size() const override;

    PlotDataPtr data() { return _plot_data; }

    void setSubsampleFactor();

    void updateData();

    PlotData::RangeTimeOpt getRangeX();

    PlotData::RangeValueOpt getRangeY(int first_index, int last_index );

    void setAlternativeAxisX( PlotDataPtr new_x_data);

    nonstd::optional<QPointF> sampleFromTime(double t);

    typedef enum{
      noTransform,
      firstDerivative,
      secondDerivative,
      XYPlot
    } Transform;

    void setTransform(Transform trans);

    Transform transform() const { return _transform; }

private:
    PlotDataPtr _plot_data;

    std::vector<QPointF> _cached_transformed_curve;

    unsigned _subsample;

    Transform _transform;

    PlotDataPtr _alternative_X_axis;

    QRectF _bounding_box;
};



#endif // PLOTDATA_H
