#include "timeseries_qwt.h"
#include <limits>
#include <stdexcept>


TimeseriesQwt::TimeseriesQwt(PlotDataPtr base):
    _plot_data(base),
    _subsample(1),
    _transform( noTransform )
{

}

QPointF TimeseriesQwt::sample(size_t i) const
{
    if(_transform == noTransform) {
        const auto& p = _plot_data->at( i );
        return QPointF( p.x, p.y) ;
    }
    else {
        return _cached_transformed_curve[i];
    }
}

QRectF TimeseriesQwt::boundingRect() const
{
    qDebug() << "boundingRect not implemented";
    return QRectF(0,0,1,1);
}


size_t TimeseriesQwt::size() const
{
    if(_transform == noTransform) {
        return _plot_data->size();
    }
    else {
        return _cached_transformed_curve.size();
    }
}


QRectF TimeseriesQwt::maximumBoundingRect(double min_X, double max_X)
{
    int x1 = _plot_data->getIndexFromX( min_X );
    int x2 = _plot_data->getIndexFromX( max_X );

    if( x1 <0 || x2 <0){
        return QRectF();
    }

    auto range_X = getRangeX();

    if( !range_X ){
        return QRectF();
    }

    auto range_Y = getRangeY( x1, x2  );
    if( !range_Y){
        return QRectF();
    }

    QRectF rect ( range_X->min,
                  range_Y->min,
                  range_X->max - range_X->min,
                  range_Y->max - range_Y->min );
    return rect;
}

void TimeseriesQwt::setSubsampleFactor()
{
    //  _subsample = (_plot_data->size() / 2000) + 1;
}

void TimeseriesQwt::updateData(bool force_transform)
{
    bool updated = _plot_data->flushAsyncBuffer();

    if(updated || force_transform)
    {
        if(_transform == firstDerivative)
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
                _cached_transformed_curve[i] = QPointF( (p1.x + p0.x)*0.5, vel) ;
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
                _cached_transformed_curve[i] =  QPointF( (p2.x + p0.x)*0.5, acc ) ;
            }
        }
        else if( _transform == XYPlot && _alternative_X_axis)
        {
            const size_t N = _alternative_X_axis->size();

            if( _plot_data->size() != N ){
                return ;
            }

            for (size_t i=0; i<N; i++ )
            {
                if( _alternative_X_axis->at(i).x != _plot_data->at(i).x ){
                    return ;
                }
            }
            _cached_transformed_curve.resize(N);
            for (size_t i=0; i<N; i++ )
            {
                _cached_transformed_curve[i] = QPointF(_alternative_X_axis->at(i).y,
                                                       _plot_data->at(i).y );
            }
        }
    }
}

PlotData::RangeTimeOpt TimeseriesQwt::getRangeX()
{   
    // std::lock_guard<std::mutex> lock(_mutex);
    if( this->size() < 2 )
        return  PlotData::RangeTimeOpt() ;
    else{
        if( _transform == XYPlot && _alternative_X_axis )
        {
            const double first_Y = _alternative_X_axis->at(0).y;
            double y_min = first_Y;
            double y_max = first_Y;

            for (int i = 1; i < _alternative_X_axis->size(); i++)
            {
                const double Y = _alternative_X_axis->at(i).y;
                if( Y < y_min )      y_min = Y;
                else if( Y > y_max ) y_max = Y;
            }
            return  PlotData::RangeTimeOpt( { y_min, y_max } );
        }
        else{
            return  PlotData::RangeTimeOpt( { sample(0).x(), sample( this->size() -1).x() } );
        }
    }
}


PlotData::RangeValueOpt TimeseriesQwt::getRangeY(int first_index, int last_index)
{
    if( first_index < 0 || last_index < 0 || first_index > last_index)
    {
        return PlotData::RangeValueOpt();
    }
    //std::lock_guard<std::mutex> lock(_mutex);

    if( _transform == XYPlot && _alternative_X_axis ){
        first_index = 0;
        last_index = size() -1;
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
        updateData(true);
    }
}
