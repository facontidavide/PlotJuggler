#include "plotlegend.h"
#include <QEvent>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>


PlotLegend::PlotLegend(QwtPlot *parent):
    _parent_plot(parent),
    _collapsed(false)
{
    setRenderHint( QwtPlotItem::RenderAntialiased );

    setMaxColumns( 1 );
    setAlignment( Qt::Alignment( Qt::AlignTop | Qt::AlignRight ) );
    setBackgroundMode( QwtPlotLegendItem::BackgroundMode::LegendBackground   );

    setBorderRadius( 2 );
    setMargin( 2 );
    setSpacing( 1 );
    setItemMargin( 2 );

    QFont font = this->font();
    font.setPointSize( 9 );
    setFont( font );
    setVisible( true );

    setBackgroundBrush(QBrush( QColor(122,122,122,40), Qt::SolidPattern));

    this->attach(parent);
}

QRectF PlotLegend::hideButtonRect() const
{
    auto canvas_rect = _parent_plot->canvas()->rect();
    if( alignment() & Qt::AlignRight)
    {
        return QRectF( geometry(canvas_rect).topRight(), QSize(8,-8) );
    }
    return QRectF( geometry(canvas_rect).topLeft(), QSize(-8,-8) );
}

void PlotLegend::draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap,
                      const QRectF &rect) const
{ 
    if( !_collapsed )
    {
        QwtPlotLegendItem::draw(painter, xMap, yMap, rect);
    }

    QRectF iconRect = hideButtonRect();

    if( isVisible() && plotItems().size() > 0)
    {
        painter->save();

        painter->setPen(  Qt::white );
        painter->setBrush( QBrush( Qt::white, Qt::SolidPattern) );
        painter->drawRect( iconRect );

        QPen black_pen (Qt::black);
        black_pen.setWidth(2);
        painter->setPen( black_pen );
        painter->drawEllipse( iconRect );
        painter->restore();
    }
}

void PlotLegend::drawLegendData( QPainter *painter,
    const QwtPlotItem *plotItem, const QwtLegendData &data,
    const QRectF &rect ) const
{
    Q_UNUSED( plotItem );

    const int m = margin();
    const QRectF r = rect.toRect().adjusted( m, m, -m, -m );

    painter->setClipRect( r, Qt::IntersectClip );

    int titleOff = 0;

    const QwtGraphic graphic = data.icon();
    if ( !graphic.isEmpty() )
    {
        QRectF iconRect( r.topLeft(), graphic.defaultSize() );

        iconRect.moveCenter(
            QPoint( iconRect.center().x(), rect.center().y() ) );

        if(  plotItem->isVisible() )
        {
            graphic.render( painter, iconRect, Qt::KeepAspectRatio );
        }

        titleOff += iconRect.width() + spacing();
    }

    const QwtText text = data.title();
    if ( !text.isEmpty() )
    {
        auto pen = textPen();
        if( !plotItem->isVisible() )
        {
            pen.setColor( QColor(122,122,122) );
        }
        else{
            pen.setColor( _parent_plot->canvas()->palette().foreground().color() );
        }
        painter->setPen( pen );
        painter->setFont( font() );

        const QRectF textRect = r.adjusted( titleOff, 0, 0, 0 );
        text.draw( painter, textRect );
    }
}


bool PlotLegend::processWheelEvent(QWheelEvent* mouse_event)
{
    if ( mouse_event->modifiers() == Qt::ControlModifier && isVisible() )
    {
        auto canvas_rect = _parent_plot->canvas()->rect();
        auto legend_rect = geometry( canvas_rect );
        if( legend_rect.contains( mouse_event->pos()) )
        {
            int point_size = font().pointSize();
            if( mouse_event->delta() >  0 && point_size < 14)
            {
                emit legendSizeChanged(point_size+1);
            }
            if( mouse_event->delta() < 0 && point_size > 6)
            {
                emit legendSizeChanged(point_size-1);
            }
            return true;
        }
    }
    return false;
}

void PlotLegend::drawBackground( QPainter *painter, const QRectF &rect ) const
{
    painter->save();

    auto pen = textPen();
    pen.setColor( _parent_plot->canvas()->palette().foreground().color() );

    painter->setPen( pen );
    painter->setBrush( backgroundBrush() );
    const double radius = borderRadius();
    painter->drawRoundedRect( rect, radius, radius );

    painter->restore();
}


const QwtPlotItem* PlotLegend::processMousePressEvent(QMouseEvent* mouse_event)
{
    auto canvas_rect = _parent_plot->canvas()->rect();
    const QPoint press_point = mouse_event->pos();

    if( isVisible() &&  mouse_event->modifiers() == Qt::NoModifier)
    {
        if( !_collapsed && geometry( canvas_rect ).contains(press_point) )
        {
            for(auto item: plotItems())
            {
                auto item_rect = legendGeometries(item).first();
                if( item_rect.contains( press_point ))
                {
                    return item;
                }
            }
        }
        else if( hideButtonRect().contains(press_point) )
        {
            _collapsed = !_collapsed;
            _parent_plot->replot();
            return nullptr;
        }
    }
    return nullptr;
}

void PlotLegend::setLegend(bool input)
{
    _collapsed = input;
    _parent_plot->replot(); 
}

bool PlotLegend::getLegend()
{
    return _collapsed;
}
