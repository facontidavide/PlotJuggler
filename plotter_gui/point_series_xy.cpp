#include "point_series_xy.h"
#include <cmath>
#include <cstdlib>


PointSeriesXY::PointSeriesXY(const PlotData *y_axis, const PlotData *x_axis):
    DataSeriesBase( &_cached_curve ),
    _x_axis(x_axis),
    _y_axis(y_axis),
    _cached_curve("")
{
    updateCache();
}

PlotData::RangeTimeOpt PointSeriesXY::getVisualizationRangeX()
{
    if( this->size() < 2 )
        return  PlotData::RangeTimeOpt();
    else{
        return PlotData::RangeTimeOpt( { _bounding_box.left() ,
                                         _bounding_box.right()} );
    }
}

nonstd::optional<QPointF> PointSeriesXY::sampleFromTime(double t)
{
    if( _cached_curve.size() == 0 )
    {
        return {};
    }

    int index = _y_axis->getIndexFromX(t);
    if(index <0)
    {
        return {};
    }
    const auto& p = _cached_curve.at( size_t(index) );
    return QPointF(p.x, p.y);
}

PlotData::RangeValueOpt PointSeriesXY::getVisualizationRangeY(PlotData::RangeTime range_X)
{
    //TODO: improve?
    return PlotData::RangeValueOpt( { _bounding_box.bottom(),
                                      _bounding_box.top() } );
}

bool PointSeriesXY::updateCache()
{
    if( _x_axis == nullptr )
    {
        throw std::runtime_error("the X axis is null");
    }

    const size_t data_size =  std::min(_x_axis->size(), _y_axis->size());
    
    if(data_size == 0)
    {
        _bounding_box = QRectF();
        _cached_curve.clear();
        return true;
    }

    double min_y =( std::numeric_limits<double>::max() );
    double max_y =(-std::numeric_limits<double>::max() );
    double min_x =( std::numeric_limits<double>::max() );
    double max_x =(-std::numeric_limits<double>::max() );

    _cached_curve.resize( data_size );

    const double EPS = std::numeric_limits<double>::epsilon();

    for (size_t i=0; i<data_size; i++ )
    {
        if( Abs( _x_axis->at(i).x  - _y_axis->at(i).x ) >  EPS)
        {
            _bounding_box = QRectF();
            _cached_curve.clear();
            throw std::runtime_error("X and Y axis don't share the same time axis");
        }

        const QPointF p(_x_axis->at(i).y,
                        _y_axis->at(i).y );

        _cached_curve.at(i) = { p.x(), p.y() };

        min_x = std::min( min_x, p.x() );
        max_x = std::max( max_x, p.x() );
        min_y = std::min( min_y, p.y() );
        max_y = std::max( max_y, p.y() );
    }

        
    _bounding_box.setLeft(  min_x );
    _bounding_box.setRight( max_x );
    _bounding_box.setBottom( min_y );
    _bounding_box.setTop( max_y );

    return true;
}
