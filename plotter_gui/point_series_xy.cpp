#include "point_series_xy.h"
#include <cmath>
#include <cstdlib>

#include <QDebug>

PointSeriesXY::PointSeriesXY(const PlotData *x_axis, const PlotData *y_axis):
    DataSeriesBase( &_cached_curve ),
    _x_axis(x_axis),
    _old_x_size(0),
    _y_axis(y_axis),
    _old_y_size(0),
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

    int y_index = _y_axis->getIndexFromX(t);
    int x_index = _x_axis->getIndexFromX(t);
    int index = std::min(x_index, y_index);
    if(index < 0)
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

// This can surely be made more efficient. 
bool PointSeriesXY::updateCache()
{
    if( _x_axis == nullptr )
    {
        throw std::runtime_error("the X axis is null");
    }
    if( _y_axis == nullptr )
    {
        throw std::runtime_error("the Y axis is null");
    }


    // Crashing here when loading a layout with placeholders and
    // then loading a bag file and opting to replace previous data.
    // Workaround (solution?) 
    // is explained in PlotWidget::updateCurves()
    const size_t data_size =  std::min(_x_axis->size(), _y_axis->size());
    
    if(data_size == 0)
    {
        _bounding_box = QRectF();
        _cached_curve.clear();
        return true;
    }

    // save a little computation
    if (_x_axis->size() == _old_x_size && _y_axis->size() == _old_y_size) 
    {
        return true;
    }
    _old_x_size = _x_axis->size();
    _old_y_size = _y_axis->size();


    double min_y =( std::numeric_limits<double>::max() );
    double max_y =(-std::numeric_limits<double>::max() );
    double min_x =( std::numeric_limits<double>::max() );
    double max_x =(-std::numeric_limits<double>::max() );

    _cached_curve.resize( data_size );

    const double EPS = std::numeric_limits<double>::epsilon();

    int count = 0;
    for (size_t i=0; i<data_size; i++ )
    {

        // The rationale for the following logic instead of just turning of this check
        // is that for data series of different sizes, we want the timing aligned.
        if( Abs( _x_axis->at(i).x  - _y_axis->at(i).x ) >  EPS)
        {
            //_bounding_box = QRectF();
            //_cached_curve.clear();
            //throw std::runtime_error("X and Y axis don't share the same time axis");
        //} 

            // Let's try aligning them instead. This could likely be accomplished
            // in a more efficient manner. Perhaps one which doesn't require the 
            // modification of sampleFromTime(). I'm imagining a function which
            // aligns _x_axis and _y_axis. I think this is the Assignment Problem, 
            // but that might be overkill.
            // This hack will do for now, but maybe not forever.
            double min = Abs( _x_axis->at(i).x  - _y_axis->at(i).x );
            size_t j=i;
            if (_x_axis->at(i).x > _y_axis->at(i).x)
            {
                while (_x_axis->at(i).x > _y_axis->at(j).x && j < data_size)
                {
                    if (Abs( _x_axis->at(i).x  - _y_axis->at(j+1).x ) < min)
                    {
                        min = Abs( _x_axis->at(i).x  - _y_axis->at(++j).x );
                    }
                    else
                    {
                        break;
                    }
                }
                const QPointF p(_x_axis->at(i).y,
                    _y_axis->at(j).y );

                _cached_curve.at(i) = { p.x(), p.y() };
                count++;

                min_x = std::min( min_x, p.x() );
                max_x = std::max( max_x, p.x() );
                min_y = std::min( min_y, p.y() );
                max_y = std::max( max_y, p.y() );
            }
            else
            {
                while (_x_axis->at(j).x < _y_axis->at(i).x && j < data_size)
                {
                    if (Abs( _x_axis->at(j+1).x  - _y_axis->at(i).x ) < min)
                    {
                        min = Abs( _x_axis->at(++j).x  - _y_axis->at(i).x );
                    }
                    else
                    {
                        break;
                    }
                }
                const QPointF p(_x_axis->at(j).y,
                    _y_axis->at(i).y );

                _cached_curve.at(i) = { p.x(), p.y() };
                count++;

                min_x = std::min( min_x, p.x() );
                max_x = std::max( max_x, p.x() );
                min_y = std::min( min_y, p.y() );
                max_y = std::max( max_y, p.y() );
            }
        }
        else
        {
            const QPointF p(_x_axis->at(i).y,
                _y_axis->at(i).y );

            _cached_curve.at(i) = { p.x(), p.y() };
            count++;

            min_x = std::min( min_x, p.x() );
            max_x = std::max( max_x, p.x() );
            min_y = std::min( min_y, p.y() );
            max_y = std::max( max_y, p.y() );
        }
    }

    _bounding_box.setLeft(  min_x );
    _bounding_box.setRight( max_x );
    _bounding_box.setBottom( min_y );
    _bounding_box.setTop( max_y );

    return true;
}
