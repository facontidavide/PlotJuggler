#include "plotwidget.h"
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <qwt_scale_widget.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_layout.h>
#include <qwt_scale_draw.h>
#include <QAction>
#include <QMessageBox>
#include <QMenu>
#include <iostream>
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
#include "PlotJuggler/random_color.h"

const double MAX_DOUBLE = std::numeric_limits<double>::max() / 2 ;

void PlotWidget::setDefaultRangeX()
{
    if( _mapped_data.numeric.size() > 0)
    {
        double min =  std::numeric_limits<double>::max();
        double max = -std::numeric_limits<double>::max();
        for (auto& it: _mapped_data.numeric )
        {
            const PlotData& data = it.second;
            if( data.size() > 0){
                double A = data.front().x;
                double B = data.back().x;
                if( A < min) min = A;
                if( B > max) max = B;
            }
        }
        this->setAxisScale( xBottom, min - _time_offset, max - _time_offset);
    }
}

void PlotWidget::changeBackgroundColor(QColor color)
{
    if( this->canvasBackground().color() != color)
    {
        this->setCanvasBackground( color );
        this->replot();
    }
}

PlotWidget::PlotWidget(PlotDataMapRef &datamap, QWidget *parent):
    QwtPlot(parent),
    _zoomer( 0 ),
    _magnifier(0 ),
    _panner( 0 ),
    _tracker ( 0 ),
    _legend( 0 ),
    _grid( 0 ),
    _mapped_data( datamap ),
    _show_line_and_points(false),
    _current_transform( TimeseriesQwt::noTransform ),
    _time_offset(0.0)
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
    connect(_zoomer,  &PlotZoomer::zoomed, this, &PlotWidget::on_externallyResized );

    _magnifier->setAxisEnabled(xTop, false);
    _magnifier->setAxisEnabled(yRight, false);

    // disable right button. keep mouse wheel
    _magnifier->setMouseButton( Qt::NoButton );
    connect(_magnifier, &PlotMagnifier::rescaled, this, &PlotWidget::on_externallyResized );
    connect(_magnifier, &PlotMagnifier::rescaled, this, &PlotWidget::replot );

    _panner->setMouseButton(  Qt::LeftButton, Qt::ControlModifier);

    //-------------------------

    buildActions();
    buildLegend();

    this->canvas()->setMouseTracking(true);
    this->canvas()->installEventFilter(this);

    setDefaultRangeX();

    _axis_limits_dialog = new AxisLimitsDialog(this);

    _custom_Y_limits.min = (-MAX_DOUBLE );
    _custom_Y_limits.max = ( MAX_DOUBLE );
}

void PlotWidget::buildActions()
{
    _action_removeCurve = new QAction(tr("&Remove curves"), this);
    _action_removeCurve->setStatusTip(tr("Remove one or more curves from this plot"));
    connect(_action_removeCurve, &QAction::triggered, this, &PlotWidget::launchRemoveCurveDialog);

    QIcon iconDelete;
    iconDelete.addFile(QStringLiteral(":/icons/resources/checkboxalt.png"), QSize(26, 26));
    _action_removeAllCurves = new QAction(tr("&Remove all curves"), this);
    _action_removeAllCurves->setIcon(iconDelete);
    connect(_action_removeAllCurves, &QAction::triggered, this, &PlotWidget::detachAllCurves);
    connect(_action_removeAllCurves, &QAction::triggered, this, &PlotWidget::undoableChange );

    QIcon iconColors;
    iconColors.addFile(QStringLiteral(":/icons/resources/office_chart_lines.png"), QSize(26, 26));
    _action_changeColorsDialog = new QAction(tr("&Change colors"), this);
    _action_changeColorsDialog->setIcon(iconColors);
    _action_changeColorsDialog->setStatusTip(tr("Change the color of the curves"));
    connect(_action_changeColorsDialog, &QAction::triggered, this, &PlotWidget::on_changeColorsDialog_triggered);

    QIcon iconPoints;
    iconPoints.addFile(QStringLiteral(":/icons/resources/line_chart_32px.png"), QSize(26, 26));
    _action_showPoints = new QAction(tr("&Show lines and points"), this);
    _action_showPoints->setIcon(iconPoints);
    _action_showPoints->setCheckable( true );
    _action_showPoints->setChecked( false );
    connect(_action_showPoints, &QAction::triggered, this, &PlotWidget::on_showPoints_triggered);

    _action_editLimits = new  QAction(tr("&Edit Axis Limits"), this);
    connect(_action_editLimits, &QAction::triggered, this, &PlotWidget::on_editAxisLimits_triggered);

    QIcon iconZoomH;
    iconZoomH.addFile(QStringLiteral(":/icons/resources/resize_horizontal.png"), QSize(26, 26));
    _action_zoomOutHorizontally = new QAction(tr("&Zoom Out Horizontally"), this);
    _action_zoomOutHorizontally->setIcon(iconZoomH);
    connect(_action_zoomOutHorizontally, &QAction::triggered, this, &PlotWidget::on_zoomOutHorizontal_triggered);
    connect(_action_zoomOutHorizontally, &QAction::triggered, this, &PlotWidget::undoableChange );

    QIcon iconZoomV;
    iconZoomV.addFile(QStringLiteral(":/icons/resources/resize_vertical.png"), QSize(26, 26));
    _action_zoomOutVertically = new QAction(tr("&Zoom Out Vertically"), this);
    _action_zoomOutVertically->setIcon(iconZoomV);
    connect(_action_zoomOutVertically, &QAction::triggered, this, &PlotWidget::on_zoomOutVertical_triggered);
    connect(_action_zoomOutVertically, &QAction::triggered, this, &PlotWidget::undoableChange );

    _action_noTransform = new QAction(tr("&NO Transform"), this);
    _action_noTransform->setCheckable( true );
    _action_noTransform->setChecked( true );
    connect(_action_noTransform, &QAction::triggered, this, &PlotWidget::on_noTransform_triggered);

    _action_1stDerivativeTransform = new QAction(tr("&1st derivative"), this);
    _action_1stDerivativeTransform->setCheckable( true );
    connect(_action_1stDerivativeTransform, &QAction::triggered, this, &PlotWidget::on_1stDerivativeTransform_triggered);

    _action_2ndDerivativeTransform = new QAction(tr("&2nd Derivative"), this);
    _action_2ndDerivativeTransform->setCheckable( true );
    connect(_action_2ndDerivativeTransform, &QAction::triggered, this, &PlotWidget::on_2ndDerivativeTransform_triggered);

    _action_phaseXY = new QAction(tr("&XY plot"), this);
    _action_phaseXY->setCheckable( true );
    connect(_action_phaseXY, &QAction::triggered, this, &PlotWidget::on_convertToXY_triggered);

    QIcon iconSave;
    iconSave.addFile(QStringLiteral(":/icons/resources/filesave@2x.png"), QSize(26, 26));
    _action_saveToFile = new  QAction(tr("&Save plot to file"), this);
    _action_saveToFile->setIcon(iconSave);
    connect(_action_saveToFile, &QAction::triggered, this, &PlotWidget::on_savePlotToFile);

    auto transform_group = new QActionGroup(this);

    transform_group->addAction(_action_noTransform);
    transform_group->addAction(_action_1stDerivativeTransform);
    transform_group->addAction(_action_2ndDerivativeTransform);
    transform_group->addAction(_action_phaseXY);
}


void PlotWidget::canvasContextMenuTriggered(const QPoint &pos)
{
    QString edit("&Edit Axis Limits ");
    edit.append( _axis_limits_dialog->limitsEnabled() ? tr("(ENABLED)") : tr("(disabled)") ) ;
    _action_editLimits->setText( edit );

    QMenu menu(this);
    menu.addAction(_action_removeCurve);
    menu.addAction(_action_removeAllCurves);
    menu.addSeparator();
    menu.addAction(_action_changeColorsDialog);
    menu.addAction(_action_showPoints);
    menu.addSeparator();
    menu.addAction(_action_editLimits);
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
    _legend->setMargin( 1 );
    _legend->setSpacing( 0 );
    _legend->setItemMargin( 0 );

    QFont font = _legend->font();
    font.setPointSize( 8 );
    _legend->setFont( font );
    _legend->setVisible( true );
}

bool PlotWidget::IsPointOnXAxis(const QPoint &p)
{
    QwtScaleWidget *bottomAxis = axisWidget(xBottom);
    if(bottomAxis)
    {
        QPoint pInAxisWidget = bottomAxis->mapFrom(this, p);
        return bottomAxis->rect().contains(pInAxisWidget);
    }
    return false;
}

void PlotWidget::drawPlotHover(PlotWidget::PlotHoverMode mode)
{
    if(mode != m_currentPlotHoverMode)
    {
        if(mode == PlotHoverMode::ON_CANVAS)
        {
            this->setCanvasBackground( QColor( 230, 230, 230 ) );
        }
        else
        {
            this->setCanvasBackground( QColor( 250, 250, 250 ) );
        }

        QwtScaleWidget *bottomAxis = axisWidget(xBottom);
        if(bottomAxis)
        {
            QPalette pal = bottomAxis->palette();
            if(mode == PlotHoverMode::ON_X_AXIS)
            {
                pal.setColor(QPalette::Background, QColor( 230, 230, 230 ));
            }
            else
            {
                pal.setColor(QPalette::Background, QColor( 250, 250, 250 ));
            }
            bottomAxis->setAutoFillBackground(true);
            bottomAxis->setPalette(pal);
        }

        replot();
        m_currentPlotHoverMode = mode;
    }
}



PlotWidget::~PlotWidget()
{

}

bool PlotWidget::addCurve(const std::string &name)
{
    auto it = _mapped_data.numeric.find( name );
    if( it == _mapped_data.numeric.end())
    {
        return false;
    }

    if( _curve_list.find(name) != _curve_list.end())
    {
        return false;
    }

    PlotData& data = it->second;

    auto curve = std::make_shared< QwtPlotCurve >( QString::fromStdString( name ) );

    TimeseriesQwt* plot_qwt = new TimeseriesQwt( &data, _time_offset );

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

    QColor color = data.getColorHint();
    if( color == Qt::black)
    {
        color = randomColorHint();
        data.setColorHint(color);
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

    return true;
}

void PlotWidget::removeCurve(const std::string &name)
{
    auto it = _curve_list.find(name);
    if( it != _curve_list.end() )
    {
        auto& curve = it->second;
        curve->detach();
        _curve_list.erase( it );

        auto marker_it = _point_marker.find(name);
        if( marker_it != _point_marker.end() )
        {
            auto marker = marker_it->second;
            if( marker ){
                marker->detach();
            }
            _point_marker.erase(marker_it);
        }

        emit curveListChanged();
    }
    if( isXYPlot() && _axisX->name() == name)
    {
        _axisX = nullptr;
        for(auto& it : _curve_list)
        {
            TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
            series->setAlternativeAxisX(_axisX);
        }
        _action_noTransform->trigger();
        emit curveListChanged();
    }

}

bool PlotWidget::isEmpty() const
{
    return _curve_list.empty();
}

const std::map<std::string, std::shared_ptr<QwtPlotCurve> > &PlotWidget::curveList() const
{
    return _curve_list;
}

void PlotWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(IsPointOnXAxis(event->pos()))
    {
        drawPlotHover(PlotHoverMode::ON_X_AXIS);
    }
    else
    {
        drawPlotHover(PlotHoverMode::ON_CANVAS);
    }

    const QMimeData *mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();
    for(const QString& format: mimeFormats)
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

void PlotWidget::dragMoveEvent(QDragMoveEvent *event)
{

    if(IsPointOnXAxis(event->pos()))
    {
        drawPlotHover(PlotHoverMode::ON_X_AXIS);
    }
    else
    {
        drawPlotHover(PlotHoverMode::ON_CANVAS);
    }
}


void PlotWidget::dragLeaveEvent(QDragLeaveEvent*)
{
    drawPlotHover(PlotHoverMode::NONE);
}

void PlotWidget::dropEvent(QDropEvent *event)
{
    drawPlotHover(PlotHoverMode::NONE);

    const QMimeData *mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();

    for(const QString& format: mimeFormats)
    {
        QByteArray encoded = mimeData->data( format );
        QDataStream stream(&encoded, QIODevice::ReadOnly);

        if( format.contains( "curveslist/add_curve") )
        {
            if(!IsPointOnXAxis(event->pos()))
            {
                bool curve_added = false;
	            while (!stream.atEnd())
	            {
	                QString curve_name;
	                stream >> curve_name;
	                bool added = addCurve( curve_name.toStdString() );
	                curve_added = curve_added || added;
	            }
	            if( curve_added )
	            {
	                zoomOut(false);
	                replot();
	                emit curveListChanged();
	                emit undoableChange();
	            }
	            event->acceptProposedAction();
            }
            else
            {
                QString curve_name;
	            stream >> curve_name;
	            changeAxisX(curve_name);
	            event->acceptProposedAction();
            }
            
        }
        else if( format.contains( "plot_area") )
        {
            QString source_name;
            stream >> source_name;
            PlotWidget* source_plot = static_cast<PlotWidget*>( event->source() );

            emit swapWidgetsRequested( source_plot, this );
            event->acceptProposedAction();
        }
    }
}

void PlotWidget::detachAllCurves()
{
    for(auto& it: _curve_list)   { it.second->detach(); }
    for(auto& it: _point_marker) { it.second->detach(); }

    if( isXYPlot() )
    {
        _axisX = nullptr;
        _action_noTransform->trigger();
    }

    _curve_list.clear();
    _point_marker.clear();
    emit _tracker->setPosition( _tracker->actualPosition() );

    emit curveListChanged();

    replot();
}

QDomElement PlotWidget::xmlSaveState( QDomDocument &doc) const
{
    QDomElement plot_el = doc.createElement("plot");

    QDomElement range_el = doc.createElement("range");
    QRectF rect = this->currentBoundingRect();
    range_el.setAttribute("bottom", QString::number(rect.bottom(), 'f', 6) );
    range_el.setAttribute("top", QString::number(rect.top(), 'f', 6));
    range_el.setAttribute("left", QString::number(rect.left(), 'f', 6));
    range_el.setAttribute("right", QString::number(rect.right() ,'f', 6));
    plot_el.appendChild(range_el);

    QDomElement limitY_el = doc.createElement("limitY");
    if( _custom_Y_limits.min > -MAX_DOUBLE){
        limitY_el.setAttribute("min", QString::number( _custom_Y_limits.min) );
    }
    if( _custom_Y_limits.max < MAX_DOUBLE){
        limitY_el.setAttribute("max", QString::number( _custom_Y_limits.max) );
    }
    plot_el.appendChild(limitY_el);

    for(auto& it: _curve_list)
    {
        auto& name = it.first;
        auto& curve = it.second;
        QDomElement curve_el = doc.createElement("curve");
        curve_el.setAttribute( "name", QString::fromStdString( name ));
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
            transform.setAttribute("axisX",  QString::fromStdString( _axisX->name() ) );
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

bool PlotWidget::xmlLoadState(QDomElement &plot_widget)
{
    QDomElement curve;

    std::set<std::string> added_curve_names;

    QDomElement transform = plot_widget.firstChildElement( "transform" );
    if( !transform.isNull()    )
    {
        if( transform.attribute("value") == "XYPlot")
        {
            QString axisX_name = transform.attribute("axisX");
            if( axisX_name.size()>0){
                changeAxisX( axisX_name );
            }
        }
    }

    QDomElement limitY_el = plot_widget.firstChildElement("limitY");
    if( !limitY_el.isNull() )
    {
        if( limitY_el.hasAttribute("min") ) {
            _custom_Y_limits.min = limitY_el.attribute("min").toDouble();
            _axis_limits_dialog->enableMin( true, _custom_Y_limits.min);
        }
        else{
            _custom_Y_limits.max = -MAX_DOUBLE;
            _axis_limits_dialog->enableMin( false, _custom_Y_limits.min);
        }

        if( limitY_el.hasAttribute("max") ) {
            _custom_Y_limits.max = limitY_el.attribute("max").toDouble();
            _axis_limits_dialog->enableMax( true, _custom_Y_limits.max);
        }
        else{
            _custom_Y_limits.max  = MAX_DOUBLE;
            _axis_limits_dialog->enableMax( false, _custom_Y_limits.max);
        }
    }

    static bool warning_message_shown = false;

    bool curve_added = false;

    for (  curve = plot_widget.firstChildElement( "curve" )  ;
           !curve.isNull();
           curve = curve.nextSiblingElement( "curve" ) )
    {
        std::string curve_name = curve.attribute("name").toStdString();
        int R = curve.attribute("R").toInt();
        int G = curve.attribute("G").toInt();
        int B = curve.attribute("B").toInt();
        QColor color(R,G,B);

        if(  _mapped_data.numeric.find(curve_name) != _mapped_data.numeric.end() )
        {
            auto added = addCurve( curve_name );
            curve_added = curve_added || added;
            _curve_list[curve_name]->setPen( color, 1.0);
            added_curve_names.insert(curve_name );
        }
        else if( ! warning_message_shown )
        {
            QMessageBox::warning(this, "Warning",
                                 tr("Can't find one or more curves.\n"
                                    "This message will be shown only once.") );
            warning_message_shown = true;
        }
    }

    bool curve_removed = true;

    while( curve_removed )
    {
        curve_removed = false;
        for(auto& it: _curve_list)
        {
            auto& curve_name = it.first;
            if( added_curve_names.find( curve_name ) == added_curve_names.end())
            {
                removeCurve( curve_name );
                curve_removed = true;
                break;
            }
        }
    }

    if( curve_removed || curve_added)
    {
        replot();
        emit curveListChanged();
    }

    //-----------------------------------------

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
        //        else if(trans_value == "XYPlot")
        //        {
        //            QString axisX_name = transform.attribute("axisX");
        //            if( axisX_name.size()>0)
        //            {
        //                changeAxisX( axisX_name );
        //            }
        //        }
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
    if( isXYPlot() )
    {
        auto it = _mapped_data.numeric.find( _axisX->name() );
        if( it != _mapped_data.numeric.end() ){
            _axisX = &(it->second);
        }
        else{
            _axisX = nullptr;
        }
    }

    for (auto& curve_it: _curve_list)
    {
        std::shared_ptr<QwtPlotCurve>& curve_data = curve_it.second;
        const auto& curve_name = curve_it.first;

        auto it = _mapped_data.numeric.find( curve_name );
        if( it != _mapped_data.numeric.end())
        {
            TimeseriesQwt* new_plotqwt = new TimeseriesQwt( &(it->second), _time_offset );
            new_plotqwt->setAlternativeAxisX( _axisX );
            new_plotqwt->setTransform( _current_transform );
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

void PlotWidget::configureTracker(CurveTracker::Parameter val)
{
    _tracker->setParameter( val );
}

void PlotWidget::enableTracker(bool enable)
{
    _tracker->setEnabled( enable && !isXYPlot() );
}

void PlotWidget::setTrackerPosition(double abs_time)
{
    if( isXYPlot()){
        for (auto& it: _curve_list)
        {
            auto& name = it.first;
            TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
            auto pointXY = series->sampleFromTime(abs_time);
            if( pointXY ){
                _point_marker[name]->setValue( pointXY.value() );
            }
        }
    }
    else{
        double relative_time = abs_time - _time_offset;
        _tracker->setPosition( QPointF( relative_time , 0.0) );
    }
}

void PlotWidget::on_changeTimeOffset(double offset)
{
    _time_offset = offset;
    for (auto& it: _curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setTimeOffset(offset);
    }
    zoomOut(false);
}


PlotData::RangeTime PlotWidget::getMaximumRangeX() const
{
    double left   =  std::numeric_limits<double>::max();
    double right  = -std::numeric_limits<double>::max();

    for(auto& it: _curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        auto range_X = series->getVisualizationRangeX();

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
    right = right + margin;
    left  = left  - margin;

    return PlotData::RangeTime( {left,right} );
}

//TODO report failure for empty dataset
PlotData::RangeValue  PlotWidget::getMaximumRangeY( PlotData::RangeTime range_X, bool absolute_time) const
{
    double top    = -std::numeric_limits<double>::max();
    double bottom =  std::numeric_limits<double>::max();

    for(auto& it: _curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );

        const auto max_range_X = series->getVisualizationRangeX();
        if( !max_range_X ) continue;

        double left  = std::max(max_range_X->min, range_X.min);
        double right = std::min(max_range_X->max, range_X.max);

        if( !absolute_time )
        {
            left += _time_offset;
            right += _time_offset;
            left = std::nextafter(left, right);
            right = std::nextafter(right, left);
        }

        int X0 = series->data()->getIndexFromX(left);
        int X1 = series->data()->getIndexFromX(right);

        if( X0<0 || X1 <0)
        {
            qDebug() << " invalid X0/X1 range in PlotWidget::maximumRangeY";
            continue;
        }
        else{
            auto range_Y = series->getVisualizationRangeY(X0, X1);
            if( !range_Y )
            {
                qDebug() << " invalid range_Y in PlotWidget::maximumRangeY";
                continue;
            }
            if( top <    range_Y->max )    top    = range_Y->max;
            if( bottom > range_Y->min )    bottom = range_Y->min;
        }
    }

    double margin = 0.1;

    if( bottom > top ){
        bottom  = 0;
        top = 0;
    }

    if( top - bottom > std::numeric_limits<double>::epsilon() )
    {
        margin = (top-bottom) * 0.025;
    }

    const bool lower_limit = _custom_Y_limits.min > -MAX_DOUBLE;
    const bool upper_limit = _custom_Y_limits.max <  MAX_DOUBLE;

    if(lower_limit)
    {
        bottom = _custom_Y_limits.min;
        if( top < bottom ) top = bottom + margin;
    }

    if( upper_limit )
    {
        top = _custom_Y_limits.max;
        if( top < bottom ) bottom = top - margin;
    }

    if( !lower_limit && !upper_limit )
    {
        top    += margin;
        bottom -= margin;
    }

    return PlotData::RangeValue({ bottom,  top});
}

void PlotWidget::updateCurves()
{
    for(auto& it: _curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
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

    for(auto& it: _curve_list)
    {
        dialog->addCurveName( QString::fromStdString( it.first ) );
    }

    dialog->exec();

    if( prev_curve_count != _curve_list.size() )
    {
        emit undoableChange();
    }
}

void PlotWidget::on_changeColorsDialog_triggered()
{
    std::map<std::string,QColor> color_by_name;

    for(auto& it: _curve_list)
    {
        const auto& curve_name = it.first;
        auto& curve = it.second;
        color_by_name.insert(std::make_pair( curve_name, curve->pen().color() ));
    }

    CurveColorPick* dialog = new CurveColorPick(color_by_name, this);

    connect( dialog, &CurveColorPick::changeColor, this, &PlotWidget::on_changeColor,
             Qt::DirectConnection);

    dialog->exec();

    if( dialog->anyColorModified() )
    {
        emit undoableChange();
    }
}

void PlotWidget::on_changeColor(QString curve_name, QColor new_color)
{
    auto it = _curve_list.find(curve_name.toStdString());
    if( it != _curve_list.end())
    {
        auto& curve = it->second;
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
    for(auto& it: _curve_list)
    {
        auto& curve = it.second;
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
    if( _curve_list.size() == 0)
    {
        QRectF rect(0, 1, 1, -1);
        this->setScale(rect, false);
        return;
    }

    QRectF rect;
    auto rangeX = getMaximumRangeX();

    rect.setLeft( rangeX.min );
    rect.setRight( rangeX.max );

    auto rangeY = getMaximumRangeY( rangeX, false );

    rect.setBottom( rangeY.min   );
    rect.setTop(  rangeY.max  );

    _magnifier->setAxisLimits( xBottom, rect.left(),   rect.right() );
    _magnifier->setAxisLimits( yLeft,   rect.bottom(), rect.top() );

    this->setScale(rect, emit_signal);
}

void PlotWidget::on_zoomOutHorizontal_triggered(bool emit_signal)
{
    QRectF act = currentBoundingRect();
    auto rangeX = getMaximumRangeX();

    act.setLeft( rangeX.min );
    act.setRight( rangeX.max );
    this->setScale(act, emit_signal);
}

void PlotWidget::on_zoomOutVertical_triggered(bool emit_signal)
{
    QRectF rect = currentBoundingRect();
    auto rangeY = getMaximumRangeY( {rect.left(), rect.right()}, false );

    rect.setBottom(  rangeY.min );
    rect.setTop(     rangeY.max );

    _magnifier->setAxisLimits( yLeft, rect.bottom(), rect.top() );

    this->setScale(rect, emit_signal);
}

void PlotWidget::on_noTransform_triggered(bool checked )
{
    enableTracker(true);
    if(_current_transform ==  TimeseriesQwt::noTransform) return;

    for(auto& it: _curve_list)
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
    enableTracker(true);
    if(_current_transform ==  TimeseriesQwt::firstDerivative) return;

    for(auto& it: _curve_list)
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
    enableTracker(true);
    if(_current_transform ==  TimeseriesQwt::secondDerivative) return;

    for(auto& it: _curve_list)
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
    return (_current_transform == TimeseriesQwt::XYPlot && _axisX);
}


void PlotWidget::on_convertToXY_triggered(bool)
{
    if( !_axisX )
    {
        QMessageBox::warning(0, tr("Warning"),
                             tr("To show a XY plot, you must first provide an alternative X axis.\n"
                                "You can do this drag'n dropping a curve on the x axis ") );

        if( _current_transform == TimeseriesQwt::noTransform)
        {
            _action_noTransform->trigger();
        }
        else if( _current_transform == TimeseriesQwt::firstDerivative)
        {
            _action_1stDerivativeTransform->trigger();
        }
        else if( _current_transform == TimeseriesQwt::secondDerivative)
        {
            _action_2ndDerivativeTransform->trigger();
        }
        return;
    }

    enableTracker(false);

    _current_transform = TimeseriesQwt::XYPlot;

    for(auto& it: _curve_list)
    {
        TimeseriesQwt* series = static_cast<TimeseriesQwt*>( it.second->data() );
        series->setAlternativeAxisX( _axisX );
        series->setTransform( TimeseriesQwt::XYPlot );
        _point_marker[ it.first ]->setVisible(true);
    }

    QFont font_footer;
    font_footer.setPointSize(10);
    QwtText text( QString::fromStdString( _axisX->name()) );
    text.setFont(font_footer);

    this->setFooter( text );

    zoomOut(true);
    replot();
}

void PlotWidget::changeAxisX(QString curve_name)
{
    auto it = _mapped_data.numeric.find( curve_name.toStdString() );
    if( it != _mapped_data.numeric.end())
    {
        _axisX = &(it->second);
        _action_phaseXY->trigger();
    }
    else{
        // do nothing (?)
    }
}

void PlotWidget::on_savePlotToFile()
{
    QString fileName;

    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("png");

#ifndef QWT_NO_SVG
    saveDialog.setNameFilter("Compatible formats (*.jpg *.jpeg *.svg *.png)");
#else
    saveDialog.setNameFilter("Compatible formats (*.jpg *.jpeg *.png)");
#endif
    saveDialog.exec();

    if(saveDialog.result() == QDialog::Accepted && !saveDialog.selectedFiles().empty())
    {
        fileName = saveDialog.selectedFiles().first();

        QPixmap pixmap (1200,900);
        QPainter * painter = new QPainter(&pixmap);

        if ( !fileName.isEmpty() )
        {
            QwtPlotRenderer rend;
            rend.render(this, painter, QRect(0, 0, pixmap.width(), pixmap.height()));
            pixmap.save(fileName);
        }
    }
}

void PlotWidget::on_editAxisLimits_triggered()
{
    auto rangeX = this->getMaximumRangeX();

    //temporary reset the limit during editing
    _custom_Y_limits.min = -MAX_DOUBLE;
    _custom_Y_limits.max =  MAX_DOUBLE;

    auto rangeY = getMaximumRangeY(rangeX, false);

    _axis_limits_dialog->setDefaultRange(rangeY);
    _axis_limits_dialog->exec();

    _custom_Y_limits = _axis_limits_dialog->rangeY();

    on_zoomOutVertical_triggered(false);
    replot();
    emit undoableChange();
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
            if( mouse_event->modifiers() == Qt::ShiftModifier) // time tracker
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
                // delegate to _zoomer
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

    case QEvent::Leave:
    {
        changeBackgroundColor( QColor( 250, 250, 250 ) );
    }break;
        //---------------------------------
    case QEvent::MouseButtonRelease :
    {
        changeBackgroundColor( QColor( 250, 250, 250 ) );
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

    case QEvent::DragEnter: {
        this->dragEnterEvent( static_cast<QDragEnterEvent*>(event) );
    } break;                         // drag moves into widget
    case QEvent::DragMove:  {
        this->dragMoveEvent(  static_cast<QDragMoveEvent*>(event ) );
    } break;
    case QEvent::DragLeave: {
        this->dragLeaveEvent( static_cast<QDragLeaveEvent*>(event ) );
    } break;
    case QEvent::Drop:      {
        this->dropEvent( static_cast<QDropEvent*>(event ) );
    } break;


    } //end switch

    return QWidget::eventFilter( obj, event );
}


