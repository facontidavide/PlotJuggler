#include "timeseries_qwt.h"
#include <limits>
#include <stdexcept>
#include <QMessageBox>
#include <QString>

TimeseriesQwt::TimeseriesQwt(PlotDataPtr base):
    _plot_data(base),
    _subsample(1),
    _transform( noTransform )
{
    updateData();
}

QPointF TimeseriesQwt::sample(size_t i) const
{
    return _cached_transformed_curve[i];
}

QRectF TimeseriesQwt::boundingRect() const
{
    return _bounding_box;
}


size_t TimeseriesQwt::size() const
{
    return _cached_transformed_curve.size();
}


void TimeseriesQwt::setSubsampleFactor()
{
    //  _subsample = (_plot_data->size() / 2000) + 1;
}

void TimeseriesQwt::updateData()
{
    if(_plot_data->size() == 0) return;

    double min_y =( std::numeric_limits<double>::max() );
    double max_y =( std::numeric_limits<double>::min() );
    double min_x =( std::numeric_limits<double>::max() );
    double max_x =( std::numeric_limits<double>::min() );

    //if(updated || force_transform)
    {
        if(_transform == noTransform)
        {
            _cached_transformed_curve.resize(_plot_data->size());
            for (size_t i=0; i< _plot_data->size(); i++ )
            {
                const auto& p = _plot_data->at( i );
                _cached_transformed_curve[i] = QPointF(p.x, p.y) ;

                if(min_y > p.y) min_y =(p.y);
                else if(max_y    < p.y) max_y =(p.y);

                if(min_x   > p.x) min_x =(p.x);
                else if(max_x  < p.x) max_x =(p.x);
            }
        }
        else if(_transform == firstDerivative)
        {
            if( _plot_data->size() < 1){
                _cached_transformed_curve.clear();
            }
            else{
                _cached_transformed_curve.resize( _plot_data->size() - 1 );
            }

            for (size_t i=0; i< _plot_data->size() -1; i++ )
            {
                const auto& p0 = _plot_data->at( i );
                const auto& p1 = _plot_data->at( i+1 );
                const auto delta = p1.x - p0.x;
                const auto vel = (p1.y - p0.y) /delta;
                const QPointF p( (p1.x + p0.x)*0.5, vel);
                _cached_transformed_curve[i] = p;

                if(min_y > p.y()) min_y =(p.y());
                else if(max_y    < p.y()) max_y =(p.y());

                if(min_x   > p.x()) min_x =(p.x());
                else if(max_x  < p.x()) max_x =(p.x());
            }
        }
        else if(_transform == secondDerivative)
        {
            if( _plot_data->size() < 2){
                _cached_transformed_curve.clear();
            }
            else{
                _cached_transformed_curve.resize( _plot_data->size() - 2 );
            }

            for (size_t i=0; i< _cached_transformed_curve.size(); i++ )
            {
                const auto& p0 = _plot_data->at( i );
                const auto& p1 = _plot_data->at( i+1 );
                const auto& p2 = _plot_data->at( i+2 );
                const auto delta = (p2.x - p0.x) *0.5;
                const auto acc = ( p2.y - 2.0* p1.y + p0.y)/(delta*delta);
                const QPointF p( (p2.x + p0.x)*0.5, acc );
                _cached_transformed_curve[i] = p;

                if(min_y > p.y()) min_y =(p.y());
                else if(max_y    < p.y()) max_y =(p.y());

                if(min_x   > p.x()) min_x =(p.x());
                else if(max_x  < p.x()) max_x =(p.x());
            }
        }
        else if( _transform == XYPlot && _alternative_X_axis)
        {
            bool failed = false;
            const size_t N = _alternative_X_axis->size();

            if( _plot_data->size() != N ){
                failed = true ;
            }

            for (size_t i=0; i<N && !failed; i++ )
            {
                if( _alternative_X_axis->at(i).x != _plot_data->at(i).x ){
                    failed = true ;
                    break;
                }
            }

            if( failed){
                QMessageBox::warning(0, QString("Warning"),
                                     QString("The creation of the XY plot failed because at least two "
                                             "timeseries don't share the same time axis.") );
                throw std::runtime_error("back to no Transform");
            }
            else{
                _cached_transformed_curve.resize(N);
                for (size_t i=0; i<N; i++ )
                {
                    const QPointF p(_alternative_X_axis->at(i).y, _plot_data->at(i).y );
                    _cached_transformed_curve[i] = p;

                    if(min_y > p.y()) min_y =(p.y());
                    else if(max_y    < p.y()) max_y =(p.y());

                    if(min_x   > p.x()) min_x =(p.x());
                    else if(max_x  < p.x()) max_x =(p.x());
                }
            }
        }
    }
    _bounding_box.setBottom(min_y);
    _bounding_box.setTop(max_y);
    _bounding_box.setLeft(min_x);
    _bounding_box.setRight(max_x);
}

PlotData::RangeTimeOpt TimeseriesQwt::getRangeX()
{   
    // std::lock_guard<std::mutex> lock(_mutex);
    if( this->size() < 2 )
        return  PlotData::RangeTimeOpt() ;
    else{
        return PlotData::RangeTimeOpt( { _bounding_box.left(), _bounding_box.right() } );
    }
}


PlotData::RangeValueOpt TimeseriesQwt::getRangeY(int first_index, int last_index)
{
    if( first_index < 0 || last_index < 0 || first_index > last_index)
    {
        return PlotData::RangeValueOpt();
    }

    if( (_transform == XYPlot && _alternative_X_axis) ||
        ( first_index==0 && last_index == size() -1) )
    {
        return PlotData::RangeValueOpt( { _bounding_box.bottom(), _bounding_box.top() } );
    }

    const double first_Y = sample(first_index).y();
    double y_min = first_Y;
    double y_max = first_Y;

    for (int i = first_index+1; i < last_index; i++)
    {
        const double Y = sample(i).y();

        if( Y < y_min )      y_min = Y;
        else if( Y > y_max ) y_max = Y;
    }
    return PlotData::RangeValueOpt( { y_min, y_max } );
}

void TimeseriesQwt::setAlternativeAxisX(PlotDataPtr new_x_data)
{
    _alternative_X_axis = new_x_data;
}

nonstd::optional<QPointF> TimeseriesQwt::sampleFromTime(double t)
{
    if( _transform == XYPlot && _alternative_X_axis)
    {
        auto res1 = _alternative_X_axis->getYfromX( t );
        if( res1)
        {
            auto res2 = _plot_data->getYfromX( t );
            if( res2 ){
                return nonstd::optional<QPointF>(
                            QPointF(res1.value(), res2.value() ) ) ;
            }
        }
    }
    else{
        auto res = _plot_data->getYfromX( t );
        if(res){
            return nonstd::optional<QPointF>( QPointF(t, res.value() ) ) ;
        }
    }
    return nonstd::optional<QPointF>();
}

void TimeseriesQwt::setTransform(TimeseriesQwt::Transform trans)
{
    if(trans != _transform)
    {
        _transform = trans;
        updateData();
    }
}
