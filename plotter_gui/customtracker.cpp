#include "customtracker.h"
#include <qwt_series_data.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "qwt_event_pattern.h"
#include <qwt_symbol.h>
#include <qevent.h>

struct compareX
{
    inline bool operator()( const double x, const QPointF &pos ) const
    {
        return ( x < pos.x() );
    }
};



CurveTracker::CurveTracker( QwtPlot *plot ):
    QObject( plot ),
    _plot( plot )
{
    _line_marker = ( new QwtPlotMarker );

    _line_marker->setLinePen(QPen(Qt::red));
    _line_marker->setLineStyle(QwtPlotMarker::VLine);
    _line_marker->setValue(0,0);
    _line_marker->attach(plot);

    _text_marker = ( new QwtPlotMarker );
    _text_marker->attach(plot);

}

QPointF CurveTracker::actualPosition() const
{
    return _prev_trackerpoint;
}

void CurveTracker::setEnabled(bool enable)
{
    _visible = enable;

    _line_marker->setVisible( enable );
    _text_marker->setVisible( enable );

}

void CurveTracker::setPosition(const QPointF& position)
{
    const QwtPlotItemList curves = _plot->itemList( QwtPlotItem::Rtti_PlotCurve );

    _line_marker->setValue( position );

    QRectF rect;
    rect.setBottom( _plot->canvasMap( QwtPlot::yLeft ).s1() );
    rect.setTop( _plot->canvasMap( QwtPlot::yLeft ).s2() );
    rect.setLeft( _plot->canvasMap( QwtPlot::xBottom ).s1() );
    rect.setRight( _plot->canvasMap( QwtPlot::xBottom ).s2() );

    double tot_Y = 0;
    int visible_points = 0;

    while( _marker.size() >  curves.size())
    {
        _marker.back()->detach();
        _marker.pop_back();
    }

    for (int i = _marker.size() ; i < curves.size(); i++ )
    {
        _marker.push_back( new QwtPlotMarker );
        _marker[i]->attach( _plot );
        _marker[i]->setVisible( _visible );
    }

    QString text_marker_info;
    double text_X_offset = 0;

    for ( int i = 0; i < curves.size(); i++ )
    {
        QwtPlotCurve *curve = static_cast<QwtPlotCurve *>(curves[i]);
        QColor color = curve->pen().color();

        text_X_offset =  rect.width() * 0.02;

        if( !_marker[i]->symbol() )
        {
            QwtSymbol *sym = new QwtSymbol(
                        QwtSymbol::Diamond,
                        color,
                        color,
                        QSize(5,5));
            _marker[i]->setSymbol(sym);
        }

        const QLineF line = curveLineAt( curve, position.x() );

        if( line.isNull() )
        {
            continue;
        }

        QPointF point;
        double middle_X = (line.p1().x() + line.p2().x()) / 2.0;

        if(  position.x() < middle_X )
            point = line.p1();
        else
            point = line.p2();

        _marker[i]->setValue( point );

        if( rect.contains( point ) &&  _visible)
        {
            tot_Y += point.y();
            visible_points++;

            text_marker_info += QString( "<font color=""%1"">%2</font>" ).arg( color.name() ).arg( point.y() );
            if(  (i+1) < curves.size() ){
                text_marker_info += "<br>";
            }
            _marker[i]->setVisible( true );
        }
        else{
            _marker[i]->setVisible( false );
        }
        _marker[i]->setValue( point );
    }

    QwtText mark_text;
    mark_text.setColor( Qt::black );

    QColor c( "#FFFFFF" );
    mark_text.setBorderPen( QPen( c, 2 ) );
    c.setAlpha( 200 );
    mark_text.setBackgroundBrush( c );
    mark_text.setText( text_marker_info );

    _text_marker->setLabel(mark_text);
    _text_marker->setLabelAlignment( Qt::AlignRight );
    _text_marker->setXValue( position.x() + text_X_offset );

    if(visible_points > 0){
        _text_marker->setYValue( tot_Y/visible_points );
    }
    _text_marker->setVisible( visible_points > 0 &&  _visible);

    _prev_trackerpoint = position;

}


QLineF CurveTracker::curveLineAt(
        const QwtPlotCurve *curve, double x ) const
{
    QLineF line;

    if ( curve->dataSize() >= 2 )
    {
        int index = qwtUpperSampleIndex<QPointF>(
                    *curve->data(), x, compareX() );

        if ( index > 0 )
        {
            line.setP1( curve->sample( index - 1 ) );
            line.setP2( curve->sample( index ) );
        }
    }
    return line;
}

