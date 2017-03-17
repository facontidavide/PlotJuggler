#include "plotwidget.h"
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <qwt_plot_canvas.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_draw.h>
#include <QAction>
#include <QMessageBox>
#include <QMenu>
#include <limits>
#include "removecurvedialog.h"
#include "curvecolorpick.h"
#include <QApplication>
#include <set>
#include <memory>
#include <qwt_text.h>
#include <QActionGroup>
#include <QFileDialog>
#include <QtXml/QDomElement>
#include "qwt_plot_renderer.h"
#include <PlotJuggler/random_color.h>


void PlotWidget::setDefaultRangeX()
{
    if( _mapped_data->numeric.size() > 0)
    {
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::min();
        for (auto it: _mapped_data->numeric )
        {
            const PlotDataPtr& data = it.second;
            if( data->size() > 0){
                double A = data->at(0).x;
                double B = data->at( data->size() -1 ).x;
                if( A < min) min = A;
                if( B > max) max = B;
            }
        }
        this->setAxisScale( xBottom, min, max);
    }
}

PlotWidget::PlotWidget(PlotDataMap *datamap, QWidget *parent):
    QwtPlot(parent),
    _zoomer( 0 ),
    _magnifier(0 ),
    _panner( 0 ),
    _tracker ( 0 ),
    _legend( 0 ),
    _grid( 0 ),
    _mapped_data( datamap ),
    _show_line_and_points(false),
    _current_transform( TimeseriesQwt::noTransform )
{
    this->setAcceptDrops( true );
    this->setMinimumWidth( 100 );
    this->setMinimumHeight( 100 );

    this->sizePolicy().setHorizontalPolicy( QSizePolicy::Expanding);
    this->sizePolicy().setVerticalPolicy( QSizePolicy::Expanding);

    QwtPlotCanvas *canvas = new QwtPlotCanvas(this);

    canvas->setFrameStyle( QFrame::NoFrame );
    canvas->setPaintAttribute( QwtPlotCanvas::BackingStore, true );

    this->setCanvas( canvas );
    this->setCanvasBackground( QColor( 250, 250, 250 ) );
    this->setAxisAutoScale(0, true);

    this->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating,true);
    this->plotLayout()->setAlignCanvasToScales( true );

    this->canvas()->installEventFilter( this );

    //--------------------------
    _grid = new QwtPlotGrid();
    _zoomer = ( new PlotZoomer( this->canvas() ) );
    _magnifier = ( new PlotMagnifier( this->canvas() ) );
    _panner = ( new QwtPlotPanner( this->canvas() ) );
    _tracker = ( new CurveTracker( this ) );

    _grid->setPen(QPen(Qt::gray, 0.0, Qt::DotLine));

    _zoomer->setRubberBandPen( QColor( Qt::red , 1, Qt::DotLine) );
    _zoomer->setTrackerPen( QColor( Qt::green, 1, Qt::DotLine ) );
    _zoomer->setMousePattern( QwtEventPattern::MouseSelect1, Qt::LeftButton, Qt::NoModifier );
    connect(_zoomer, SIGNAL(zoomed(const QRectF&)), this, SLOT(on_externallyResized(const QRectF&)) );

    _magnifier->setAxisEnabled(xTop, false);
    _magnifier->setAxisEnabled(yRight, false);

    // disable right button. keep mouse wheel
    _magnifier->setMouseButton( Qt::NoButton );
    connect(_magnifier, SIGNAL(rescaled(const QRectF&)), this, SLOT(on_externallyResized(const QRectF&)) );
    connect(_magnifier, SIGNAL(rescaled(const QRectF&)), this, SLOT(replot()) );

    _panner->setMouseButton(  Qt::LeftButton, Qt::ControlModifier);

    //-------------------------

    buildActions();
    buildLegend();

    this->canvas()->setMouseTracking(true);
    this->canvas()->installEventFilter(this);

    setDefaultRangeX();
}

void PlotWidget::buildActions()
{
    _action_removeCurve = new QAction(tr("&Remove curves"), this);
    _action_removeCurve->setStatusTip(tr("Remove one or more curves from this plot"));
    connect(_action_removeCurve, SIGNAL(triggered()), this, SLOT(launchRemoveCurveDialog()));

    QIcon iconDelete;
    iconDelete.addFile(QStringLiteral(":/icons/resources/checkboxalt.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_removeAllCurves = new QAction(tr("&Remove all curves"), this);
    _action_removeAllCurves->setIcon(iconDelete);
    connect(_action_removeAllCurves, SIGNAL(triggered()), this, SLOT(detachAllCurves()));
    connect(_action_removeAllCurves, SIGNAL(triggered()), this, SIGNAL(undoableChange()) );

    QIcon iconColors;
    iconColors.addFile(QStringLiteral(":/icons/resources/office_chart_lines.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_changeColorsDialog = new QAction(tr("&Change colors"), this);
    _action_changeColorsDialog->setIcon(iconColors);
    _action_changeColorsDialog->setStatusTip(tr("Change the color of the curves"));
    connect(_action_changeColorsDialog, SIGNAL(triggered()), this, SLOT(on_changeColorsDialog_triggered()));

    QIcon iconPoints;
    iconPoints.addFile(QStringLiteral(":/icons/resources/line_chart_32px.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_showPoints = new QAction(tr("&Show lines and points"), this);
    _action_showPoints->setIcon(iconPoints);
    _action_showPoints->setCheckable( true );
    _action_showPoints->setChecked( false );
    connect(_action_showPoints, SIGNAL(triggered(bool)), this, SLOT(on_showPoints_triggered(bool)));

    QIcon iconZoomH;
    iconZoomH.addFile(QStringLiteral(":/icons/resources/resize_horizontal.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_zoomOutHorizontally = new QAction(tr("&Zoom Out Horizontally"), this);
    _action_zoomOutHorizontally->setIcon(iconZoomH);
    connect(_action_zoomOutHorizontally, SIGNAL(triggered()), this, SLOT(on_zoomOutHorizontal_triggered()));
    connect(_action_zoomOutHorizontally, SIGNAL(triggered()), this, SIGNAL(undoableChange()) );

    QIcon iconZoomV;
    iconZoomV.addFile(QStringLiteral(":/icons/resources/resize_vertical.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_zoomOutVertically = new QAction(tr("&Zoom Out Vertically"), this);
    _action_zoomOutVertically->setIcon(iconZoomV);
    connect(_action_zoomOutVertically, SIGNAL(triggered()), this, SLOT(on_zoomOutVertical_triggered()));
    connect(_action_zoomOutVertically, SIGNAL(triggered()), this, SIGNAL(undoableChange()) );

    _action_noTransform = new QAction(tr("&NO Transform"), this);
    _action_noTransform->setCheckable( true );
    _action_noTransform->setChecked( true );
    connect(_action_noTransform, SIGNAL(triggered(bool)), this, SLOT(on_noTransform_triggered(bool)));

    _action_1stDerivativeTransform = new QAction(tr("&1st derivative"), this);
    _action_1stDerivativeTransform->setCheckable( true );
    connect(_action_1stDerivativeTransform, SIGNAL(triggered(bool)), this, SLOT(on_1stDerivativeTransform_triggered(bool)));

    _action_2ndDerivativeTransform = new QAction(tr("&2nd Derivative"), this);
    _action_2ndDerivativeTransform->setCheckable( true );
    connect(_action_2ndDerivativeTransform, SIGNAL(triggered(bool)), this, SLOT(on_2ndDerivativeTransform_triggered(bool)));

    _action_phaseXY = new QAction(tr("&XY plot"), this);
    _action_phaseXY->setCheckable( true );
    connect(_action_phaseXY, SIGNAL(triggered(bool)), this, SLOT(on_convertToXY_triggered(bool)));

    QIcon iconSave;
    iconSave.addFile(QStringLiteral(":/icons/resources/filesave@2x.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_saveToFile = new  QAction(tr("&Save plot to file"), this);
    _action_saveToFile->setIcon(iconSave);
    connect(_action_saveToFile, SIGNAL(triggered()), this, SLOT(on_savePlotToFile()));

    auto transform_group = new QActionGroup(this);

    transform_group->addAction(_action_noTransform);
    transform_group->addAction(_action_1stDerivativeTransform);
    transform_group->addAction(_action_2ndDerivativeTransform);
    transform_group->addAction(_action_phaseXY);
}

void PlotWidget::buildLegend()
{
    _legend = new QwtPlotLegendItem();
    _legend->attach( this );

    _legend->setRenderHint( QwtPlotItem::RenderAntialiased );
    QColor color( Qt::black );
    _legend->setTextPen( color );
    _legend->setBorderPen( color );
    QColor c( Qt::white );
    c.setAlpha( 200 );
    _legend->setBackgroundBrush( c );

    _legend->setMaxColumns( 1 );
    _legend->setAlignment( Qt::Alignment( Qt::AlignTop | Qt::AlignRight ) );
    _legend->setBackgroundMode( QwtPlotLegendItem::BackgroundMode::LegendBackground   );

    _legend->setBorderRadius( 6 );
    _legend->setMargin( 5 );
    _legend->setSpacing( 0 );
    _legend->setItemMargin( 0 );

    QFont font = _legend->font();
    font.setPointSize( 8 );
    _legend->setFont( font );
    _legend->setVisible( true );
}



PlotWidget::~PlotWidget()
{

}

bool PlotWidget::addCurve(const QString &name, bool do_replot)
{
    auto it = _mapped_data->numeric.find( name.toStdString() );
    if( it == _mapped_data->numeric.end())
    {
        return false;
    }

    if( _curve_list.find(name) != _curve_list.end())
    {
        return false;
    }

    PlotDataPtr data = it->second;

    {
        auto curve = std::shared_ptr< QwtPlotCurve >( new QwtPlotCurve(name) );

        TimeseriesQwt* plot_qwt = new TimeseriesQwt( data );

        curve->setPaintAttribute( QwtPlotCurve::ClipPolygons, true );
        curve->setPaintAttribute( QwtPlotCurve::FilterPointsAggressive, true );

        plot_qwt->setAlternativeAxisX( _axisX );

        if( _current_transform != TimeseriesQwt::noTransform)
        {
            plot_qwt->setTransform( _current_transform );
        }

        curve->setData( plot_qwt );

        if( _show_line_and_points ) {
            curve->setStyle( QwtPlotCurve::LinesAndDots);
        }
        else{
            curve->setStyle( QwtPlotCurve::Lines);
        }

        QColor color = data->getColorHint();
        if( color == Qt::black)
        {
            color = randomColorHint();
            data->setColorHint(color);
        }
        curve->setPen( color,  0.8 );
        curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );

        curve->attach( this );
        _curve_list.insert( std::make_pair(name, curve));

        auto marker = new QwtPlotMarker;
        _point_marker.insert( std::make_pair(name, marker) );
        marker->attach( this );
        marker->setVisible( isXYPlot() );

        QwtSymbol *sym = new QwtSymbol(
                    QwtSymbol::Diamond,
                    Qt::red, color,
                    QSize(10,10));

        marker->setSymbol(sym);
    }

    auto rangeX = maximumRangeX();
    auto rangeY = maximumRangeY(rangeX);

    this->setAxisScale(xBottom, rangeX.min, rangeX.max );
    this->setAxisScale(yLeft,   rangeY.min, rangeY.max );

    if( do_replot )
    {
        replot();
    }

    return true;
}

void PlotWidget::removeCurve(const QString &name)
{
    auto it = _curve_list.find(name);
    if( it != _curve_list.end() )
    {
        auto curve = it->second;
        curve->detach();
        replot();
        _curve_list.erase( it );

        _point_marker[name]->detach();
        _point_marker.erase( name );
    }
    if( _axisX && _axisX->name() == name.toStdString())
    {
        _axisX = PlotDataPtr();
        for(auto it : _curve_list)
        {
            TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
            series->setAlternativeAxisX(_axisX);
        }
        _action_noTransform->trigger();
    }
}

bool PlotWidget::isEmpty() const
{
    return _curve_list.empty();
}

const std::map<QString, std::shared_ptr<QwtPlotCurve> > &PlotWidget::curveList() const
{
    return _curve_list;
}

void PlotWidget::dragEnterEvent(QDragEnterEvent *event)
{
    QwtPlot::dragEnterEvent(event);

    const QMimeData *mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();
    foreach(QString format, mimeFormats)
    {
        QByteArray encoded = mimeData->data( format );
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        if( format.contains( "curveslist") )
        {
            event->acceptProposedAction();
        }
        if( format.contains( "plot_area")  )
        {
            QString source_name;
            stream >> source_name;

            if(QString::compare( windowTitle(),source_name ) != 0 ){
                event->acceptProposedAction();
            }
        }
    }
}
void PlotWidget::dragMoveEvent(QDragMoveEvent *)
{

}


void PlotWidget::dropEvent(QDropEvent *event)
{
    QwtPlot::dropEvent(event);

    const QMimeData *mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();

    foreach(QString format, mimeFormats)
    {
        QByteArray encoded = mimeData->data( format );
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        if( format.contains( "curveslist/add_curve") )
        {
            bool plot_added = false;
            while (!stream.atEnd())
            {
                QString curve_name;
                stream >> curve_name;
                addCurve( curve_name, true );
                plot_added = true;
            }
            if( plot_added ) {
                emit undoableChange();
            }
        }
        else if( format.contains( "curveslist/new_X_axis") )
        {
            QString curve_name;
            stream >> curve_name;
            changeAxisX(curve_name);
        }
        else if( format.contains( "plot_area") )
        {
            QString source_name;
            stream >> source_name;
            PlotWidget* source_plot = static_cast<PlotWidget*>( event->source() );
            emit swapWidgetsRequested( source_plot, this );
        }
    }
}

void PlotWidget::detachAllCurves()
{
    for(auto it: _curve_list)   { it.second->detach(); }
    for(auto it: _point_marker) { it.second->detach(); }

    _curve_list.erase(_curve_list.begin(), _curve_list.end());
    _point_marker.erase(_point_marker.begin(), _point_marker.end());
    emit _tracker->setPosition( _tracker->actualPosition() );
    replot();
}

QDomElement PlotWidget::xmlSaveState( QDomDocument &doc) const
{
    QDomElement plot_el = doc.createElement("plot");

    QDomElement range_el = doc.createElement("range");
    QRectF rect = this->currentBoundingRect();
    range_el.setAttribute("bottom", QString::number(rect.bottom()) );
    range_el.setAttribute("top", QString::number(rect.top()) );
    range_el.setAttribute("left", QString::number(rect.left()) );
    range_el.setAttribute("right", QString::number(rect.right()) );
    plot_el.appendChild(range_el);

    for(auto it=_curve_list.begin(); it != _curve_list.end(); ++it)
    {
        QString name = it->first;
        auto curve = it->second;
        QDomElement curve_el = doc.createElement("curve");
        curve_el.setAttribute( "name",name);
        curve_el.setAttribute( "R", curve->pen().color().red());
        curve_el.setAttribute( "G", curve->pen().color().green());
        curve_el.setAttribute( "B", curve->pen().color().blue());

        plot_el.appendChild(curve_el);
    }

    QDomElement transform  = doc.createElement("transform");

    switch(_current_transform)
    {
    case TimeseriesQwt::firstDerivative:
        transform.setAttribute("value", "firstDerivative" ); break;

    case TimeseriesQwt::secondDerivative:
        transform.setAttribute("value", "secondDerivative" ); break;

    case TimeseriesQwt::noTransform:
        transform.setAttribute("value", "noTransform" ); break;

    case TimeseriesQwt::XYPlot:{

        if( _axisX ){
            transform.setAttribute("value", "XYPlot" );
            transform.setAttribute("axisX",  _axisX->name().c_str() );
        }
        else{
            transform.setAttribute("value", "noTransform" );
            transform.setAttribute("axisX",  "" );
        }
    }break;

    }
    plot_el.appendChild(transform);

    return plot_el;
}

bool PlotWidget::xmlLoadState(QDomElement &plot_widget, QMessageBox::StandardButton* answer)
{
    QDomElement curve;

    std::set<QString> added_curve_names;

    for (  curve = plot_widget.firstChildElement( "curve" )  ;
           !curve.isNull();
           curve = curve.nextSiblingElement( "curve" ) )
    {
        QString curve_name = curve.attribute("name");
        int R = curve.attribute("R").toInt();
        int G = curve.attribute("G").toInt();
        int B = curve.attribute("B").toInt();
        QColor color(R,G,B);

        if(  _mapped_data->numeric.find(curve_name.toStdString()) != _mapped_data->numeric.end() )
        {
            addCurve(curve_name, false);
            _curve_list[curve_name]->setPen( color, 1.0);
            added_curve_names.insert(curve_name );
        }
        else{
            if( *answer !=  QMessageBox::YesToAll)
            {
                *answer = QMessageBox::question(
                            0,
                            tr("Warning"),
                            tr("Can't find the curve with name %1.\n Do you want to ignore it? ").arg(curve_name),
                            QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Abort ,
                            QMessageBox::Abort );
            }

            if( *answer ==  QMessageBox::Yes || *answer ==  QMessageBox::YesToAll) {
                continue;
            }

            if( *answer ==  QMessageBox::Abort) {
                return false;
            }
        }
    }

    bool curve_removed = true;

    while( curve_removed)
    {
        curve_removed = false;
        for(auto& it: _curve_list)
        {
            QString curve_name = it.first;
            if( added_curve_names.find( curve_name ) == added_curve_names.end())
            {
                removeCurve( curve_name );
                curve_removed = true;
                break;
            }
        }
    }

    //-----------------------------------------
    QDomElement transform = plot_widget.firstChildElement( "transform" );
    if( !transform.isNull()  )
    {
        QString trans_value = transform.attribute("value");
        if( trans_value == "firstDerivative")
        {
            _action_1stDerivativeTransform->trigger();
        }
        else if(trans_value == "secondDerivative")
        {
            _action_2ndDerivativeTransform->trigger();
        }
        else if(trans_value == "noTransform")
        {
            _action_noTransform->trigger();
        }
        else if(trans_value == "XYPlot")
        {
            QString axisX_name = transform.attribute("axisX");
            if( axisX_name.size()>0)
            {
                changeAxisX( axisX_name );
            }
        }
    }
    //-----------------------------------------

    QDomElement rectangle = plot_widget.firstChildElement( "range" );
    if( !rectangle.isNull()){
        QRectF rect;
        rect.setBottom( rectangle.attribute("bottom").toDouble());
        rect.setTop( rectangle.attribute("top").toDouble());
        rect.setLeft( rectangle.attribute("left").toDouble());
        rect.setRight( rectangle.attribute("right").toDouble());
        this->setScale( rect, false);
    }

    return true;
}


QRectF PlotWidget::currentBoundingRect() const
{
    QRectF rect;
    rect.setBottom( this->canvasMap( yLeft ).s1() );
    rect.setTop( this->canvasMap( yLeft ).s2() );

    rect.setLeft( this->canvasMap( xBottom ).s1() );
    rect.setRight( this->canvasMap( xBottom ).s2() );

    return rect;
}

void PlotWidget::setScale(QRectF rect, bool emit_signal)
{
    this->setAxisScale( yLeft, rect.bottom(), rect.top());
    this->setAxisScale( xBottom, rect.left(), rect.right());
    this->updateAxes();

    if( emit_signal )
    {
        if( isXYPlot()) {
            emit undoableChange();
        }
        else{
            emit rectChanged(this, rect);
        }
    }
}

void PlotWidget::reloadPlotData()
{
    if( _axisX )
    {
        auto it = _mapped_data->numeric.find( _axisX->name() );
        if( it != _mapped_data->numeric.end() ){
            _axisX = it->second;
        }
        else{
            _axisX = PlotDataPtr();
        }
    }

    for (auto& curve_it: _curve_list)
    {
        std::shared_ptr<QwtPlotCurve>& curve_data = curve_it.second;
        const std::string curve_name = curve_it.first.toStdString();

        auto it = _mapped_data->numeric.find( curve_name );
        if( it!= _mapped_data->numeric.end())
        {
            TimeseriesQwt* new_plotqwt = new TimeseriesQwt( it->second );
            new_plotqwt->setTransform( _current_transform );
            new_plotqwt->setAlternativeAxisX( _axisX );
            curve_data->setData( new_plotqwt );
        }
    }

    if( _curve_list.size() == 0){
        setDefaultRangeX();
    }
}

void PlotWidget::activateLegent(bool activate)
{
    if( activate ) _legend->attach(this);
    else           _legend->detach();
}

void PlotWidget::activateGrid(bool activate)
{
    _grid->enableX(activate);
    _grid->enableXMin(activate);
    _grid->enableY(activate);
    _grid->enableYMin(activate);
    _grid->attach(this);
}

void PlotWidget::activateTracker(bool activate)
{
    _tracker->setEnabled( activate && isXYPlot() == false);
}

void PlotWidget::setTrackerPosition(QPointF point)
{
    if( isXYPlot()){
        double time = point.x();
        for (auto it: _curve_list)
        {
            QString name = it.first;
            TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
            auto pointXY = series->sampleFromTime(time);
            if( pointXY ){
                _point_marker[name]->setValue( pointXY.value() );
            }
        }
    }
    else{
        _tracker->setPosition( point );
    }
}


PlotData::RangeTime PlotWidget::maximumRangeX() const
{
    double left   = std::numeric_limits<double>::max();
    double right  = std::numeric_limits<double>::min();

    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it->second->data() );
        auto range_X = series->getRangeX();

        if( !range_X ) continue;

        if( left  > range_X->min )    left  = range_X->min;
        if( right < range_X->max )    right = range_X->max;
    }

    if( left > right ){
        left  = 0;
        right = 0;
    }

    double margin = 0.1;
    if( fabs(right - left) > std::numeric_limits<double>::epsilon() )
    {
        margin = isXYPlot() ? ((right-left) * 0.025) : 0.0;
    }
    right += margin;
    left  -= margin;

    _magnifier->setAxisLimits( xBottom, left, right);
    return PlotData::RangeTime( {left,right} );
}

//TODO report failure for empty dataset
PlotData::RangeValue  PlotWidget::maximumRangeY( PlotData::RangeTime range_X) const
{
    double top    = std::numeric_limits<double>::min();
    double bottom = std::numeric_limits<double>::max();

    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it->second->data() );

        const auto max_range_X = series->getRangeX();
        if( !max_range_X ) continue;

        int X0 = series->data()->getIndexFromX(std::max(max_range_X->min, range_X.min));
        int X1 = series->data()->getIndexFromX(std::min(max_range_X->max, range_X.max));

        if( X0<0 || X1 <0)
        {
            qDebug() << " invalid X0/X1 range in PlotWidget::maximumRangeY";
            continue;
        }
        else{
            auto range_Y = series->getRangeY(X0, X1);
            if( !range_Y )
            {
                qDebug() << " invalid range_Y in PlotWidget::maximumRangeY";
                continue;
            }
            if( top <    range_Y->max )    top    = range_Y->max;
            if( bottom > range_Y->min )    bottom = range_Y->min;
        }
    }

    if( bottom > top ){
        bottom  = 0;
        top = 0;
    }

    double margin = 0.1;
    if( top - bottom > std::numeric_limits<double>::epsilon() )
    {
        margin = (top-bottom) * 0.025;
    }
    top    += margin;
    bottom -= margin;

    _magnifier->setAxisLimits( yLeft, bottom, top);
    return PlotData::RangeValue({ bottom,  top});
}

void PlotWidget::updateCurves(bool force)
{
    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it->second->data() );
        series->updateData();
    }
}


void PlotWidget::replot()
{
    if( _zoomer )
        _zoomer->setZoomBase( false );

    QwtPlot::replot();
}

void PlotWidget::launchRemoveCurveDialog()
{
    RemoveCurveDialog* dialog = new RemoveCurveDialog(this);
    auto prev_curve_count = _curve_list.size();

    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        dialog->addCurveName( it->first );
    }

    dialog->exec();

    if( prev_curve_count != _curve_list.size() )
    {
        emit undoableChange();
    }
}

void PlotWidget::on_changeColorsDialog_triggered()
{
    std::map<QString,QColor> color_by_name;

    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        const QString& curve_name = it->first;
        auto curve = it->second;
        color_by_name.insert(std::make_pair( curve_name, curve->pen().color() ));
    }

    CurveColorPick* dialog = new CurveColorPick(color_by_name, this);

    connect( dialog, SIGNAL(changeColor(QString,QColor)),
             this, SLOT(on_changeColor(QString,QColor)),
             Qt::DirectConnection);

    dialog->exec();

    if( dialog->anyColorModified() )
    {
        emit undoableChange();
    }
}

void PlotWidget::on_changeColor(QString curve_name, QColor new_color)
{
    auto it = _curve_list.find(curve_name);
    if( it != _curve_list.end())
    {
        auto curve = it->second;
        if( curve->pen().color() != new_color)
        {
            curve->setPen( new_color, 1.0 );
        }
        replot();
    }
}

void PlotWidget::on_showPoints_triggered(bool checked)
{
    _show_line_and_points = checked;
    for(auto it = _curve_list.begin(); it != _curve_list.end(); ++it)
    {
        auto curve = it->second;
        if( _show_line_and_points )
        {
            curve->setStyle( QwtPlotCurve::LinesAndDots);
        }
        else{
            curve->setStyle( QwtPlotCurve::Lines);
        }
    }
    replot();
}

void PlotWidget::on_externallyResized(const QRectF& rect)
{
    if( _current_transform != TimeseriesQwt::XYPlot)
    {
        emit rectChanged(this, rect);
    }
    else{
        emit undoableChange();
    }
}


void PlotWidget::zoomOut(bool emit_signal)
{
    QRectF rect;
    auto rangeX = maximumRangeX();

    rect.setLeft( rangeX.min );
    rect.setRight( rangeX.max );

    auto rangeY = maximumRangeY( rangeX );

    rect.setBottom( rangeY.min );
    rect.setTop( rangeY.max );
    this->setScale(rect, emit_signal);
}

void PlotWidget::on_zoomOutHorizontal_triggered(bool emit_signal)
{
    QRectF act = currentBoundingRect();
    auto rangeX = maximumRangeX();

    act.setLeft( rangeX.min );
    act.setRight( rangeX.max );
    this->setScale(act, emit_signal);
}

void PlotWidget::on_zoomOutVertical_triggered(bool emit_signal)
{
    QRectF act = currentBoundingRect();
    auto rangeY = maximumRangeY( {act.left(), act.right()} );

    act.setBottom( rangeY.min );
    act.setTop( rangeY.max );
    this->setScale(act, emit_signal);
}

void PlotWidget::on_noTransform_triggered(bool checked )
{
    if(_current_transform ==  TimeseriesQwt::noTransform) return;

    for (auto it :_curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setTransform( TimeseriesQwt::noTransform );
        _point_marker[ it.first ]->setVisible(false);
    }
    this->setFooter("");
    _current_transform = ( TimeseriesQwt::noTransform );

    zoomOut(true);
    replot();
}

void PlotWidget::on_1stDerivativeTransform_triggered(bool checked)
{
    if(_current_transform ==  TimeseriesQwt::firstDerivative) return;

    for (auto it :_curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setTransform( TimeseriesQwt::firstDerivative );
        _point_marker[ it.first ]->setVisible(false);
    }

    QFont font_title;
    font_title.setPointSize(10);
    QwtText text("1st derivative");
    text.setFont(font_title);

    this->setFooter(text);
    _current_transform = ( TimeseriesQwt::firstDerivative );

    zoomOut(true);
    replot();
}

void PlotWidget::on_2ndDerivativeTransform_triggered(bool checked)
{
    if(_current_transform ==  TimeseriesQwt::secondDerivative) return;

    for (auto it :_curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setTransform( TimeseriesQwt::secondDerivative );
        _point_marker[ it.first ]->setVisible(false);
    }

    QFont font_title;
    font_title.setPointSize(10);
    QwtText text("2nd derivative");
    text.setFont(font_title);

    this->setFooter(text);
    _current_transform = ( TimeseriesQwt::secondDerivative );

    zoomOut(true);
    replot();
}

bool PlotWidget::isXYPlot() const
{
    return (_current_transform == TimeseriesQwt::XYPlot);
}

void PlotWidget::changeAxisX(QString curve_name)
{
    qDebug() << "changeAxisX " << curve_name;
    auto it = _mapped_data->numeric.find( curve_name.toStdString() );
    if( it != _mapped_data->numeric.end())
    {
        _axisX = it->second;
        _action_phaseXY->trigger();
    }
    else{
        // do nothing (?)
    }
}


void PlotWidget::on_convertToXY_triggered(bool)
{
    if( !_axisX )
    {
        QMessageBox::warning(0, tr("Warning"),
                             tr("To show a XY plot, you must first provide an alternative X axis.\n"
                                "You can do this drag'n dropping a curve using the RIGHT mouse button "
                                "instead of the left mouse button.") );
        return;
    }

    _tracker->setEnabled(false);

    qDebug() << "on_convertToXY_triggered ";
    _current_transform = TimeseriesQwt::XYPlot;

    for (auto it :_curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setAlternativeAxisX( _axisX );
        series->setTransform( TimeseriesQwt::XYPlot );
        _point_marker[ it.first ]->setVisible(true);
    }

    QFont font_footer;
    font_footer.setPointSize(10);
    QwtText text( _axisX->name().c_str() );
    text.setFont(font_footer);

    this->setFooter( text );

    zoomOut(true);
    replot();
}

void PlotWidget::on_savePlotToFile()
{
    QString fileName;

#ifndef QWT_NO_SVG
    fileName = QFileDialog::getSaveFileName(this, tr("File to export"), QString(),"Compatible formats (*.jpg *.jpeg *.svg *.png)");
#else
    fileName = QFileDialog::getSaveFileName(this, tr("File to export"), QString(),"Compatible formats (*.jpg *.jpeg *.png)");
#endif

    if ( !fileName.isEmpty() )
    {
        QwtPlotRenderer rend;
        rend.renderDocument(this,fileName, QSizeF(200,150), 150);
    }
}


void PlotWidget::canvasContextMenuTriggered(const QPoint &pos)
{
    QMenu menu(this);
    menu.addAction(_action_removeCurve);
    menu.addAction(_action_removeAllCurves);
    menu.addSeparator();
    menu.addAction(_action_changeColorsDialog);
    menu.addAction(_action_showPoints);
    menu.addSeparator();
    menu.addAction(_action_zoomOutHorizontally);
    menu.addAction(_action_zoomOutVertically);
    menu.addSeparator();
    menu.addAction( _action_noTransform );
    menu.addAction( _action_1stDerivativeTransform );
    menu.addAction( _action_2ndDerivativeTransform );
    menu.addAction( _action_phaseXY );
    menu.addSeparator();
    menu.addAction( _action_saveToFile );

    _action_removeCurve->setEnabled( ! _curve_list.empty() );
    _action_removeAllCurves->setEnabled( ! _curve_list.empty() );
    _action_changeColorsDialog->setEnabled(  ! _curve_list.empty() );

    menu.exec( canvas()->mapToGlobal(pos) );
}

bool PlotWidget::eventFilter(QObject *obj, QEvent *event)
{
    switch( event->type() )
    {

    case QEvent::MouseButtonPress:
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

        if( mouse_event->button() == Qt::LeftButton )
        {
            if( mouse_event->modifiers() == Qt::ShiftModifier) // vertical tracker
            {
                const QPoint point = mouse_event->pos();
                QPointF pointF ( invTransform( xBottom, point.x()),
                                 invTransform( yLeft, point.y()) );
                emit trackerMoved(pointF);
            }
            if( mouse_event->modifiers() == Qt::ControlModifier) // panner
            {
                QApplication::setOverrideCursor(QCursor(QPixmap(":/icons/resources/move.png")));
            }
            else if(mouse_event->modifiers() == Qt::NoModifier )
            {
                QApplication::setOverrideCursor(QCursor(QPixmap(":/icons/resources/zoom_in_32px.png")));
            }
        }
        else if( mouse_event->button() == Qt::RightButton )
        {
            if( mouse_event->modifiers() == Qt::NoModifier) // show menu
            {
                canvasContextMenuTriggered( mouse_event->pos() );
            }
            else if( mouse_event->modifiers() == Qt::ControlModifier) // Start swapping two plots
            {

                QDrag *drag = new QDrag(this);
                QMimeData *mimeData = new QMimeData;

                QByteArray data;
                QDataStream dataStream(&data, QIODevice::WriteOnly);

                dataStream << this->windowTitle();

                mimeData->setData("plot_area", data );
                drag->setMimeData(mimeData);
                drag->exec();
            }
        }
    }break;
    //---------------------------------
    case QEvent::MouseMove:
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

        if ( mouse_event->buttons() == Qt::LeftButton &&
             mouse_event->modifiers() == Qt::ShiftModifier )
        {
            const QPoint point = mouse_event->pos();
            QPointF pointF ( invTransform( xBottom, point.x()),
                             invTransform( yLeft, point.y()) );
            emit trackerMoved(pointF);
        }
    }break;
    //---------------------------------
    case QEvent::MouseButtonRelease :
    {
        QApplication::restoreOverrideCursor();
    }break;
    //---------------------------------
    case QEvent::KeyPress:
    {
//        QKeyEvent *key_event = static_cast<QKeyEvent*>(event);
//        qDebug() << key_event->key();
     }break;
    //---------------------------------
    case QEvent::Paint :
    {
        if ( obj == this->canvas())
        {
            if ( !_fps_timeStamp.isValid() )
            {
                _fps_timeStamp.start();
                _fps_counter = 0;
            }
            else{
                _fps_counter++;

                const double elapsed = _fps_timeStamp.elapsed() / 1000.0;
                if ( elapsed >= 1 )
                {
                    QFont font_title;
                    font_title.setPointSize(9);
                    QwtText fps;
                    fps.setText( QString::number( qRound( _fps_counter / elapsed ) ) );
                    fps.setFont(font_title);
                    //qDebug() << _fps_counter / elapsed ;
                    _fps_counter = 0;
                    _fps_timeStamp.start();
                }
            }
        }
    }break;

    } //end switch

    return QwtPlot::eventFilter( obj, event );
}

