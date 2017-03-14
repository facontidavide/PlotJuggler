#include <qlayout.h>
#include <qpen.h>
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include "plotmatrix.h"
#include "customtracker.h"

static int widget_uid = 0;

PlotMatrix::PlotMatrix(QString name, PlotDataMap *datamap, QWidget *parent ):
    QFrame( parent ),
    _mapped_data(datamap),
    _name(name)
{
    _num_rows = 0;
    _num_cols = 0;
    _layout = new QGridLayout( this );
    _horizontal_link = true;
    updateLayout();
}


PlotWidget* PlotMatrix::addPlotWidget(unsigned row, unsigned col)
{
    PlotWidget *plot = new PlotWidget( _mapped_data, this );

    plot->setWindowTitle(QString("PlotWidget ") + QString::number(widget_uid++));

    connect( plot, SIGNAL(rectChanged(PlotWidget*,QRectF)),
             this, SLOT(on_singlePlotScaleChanged(PlotWidget*,QRectF)));

    plot->setAttribute(Qt::WA_DeleteOnClose);

    _layout->addWidget( plot, row, col );
    _layout->setRowStretch(row,1);
    _layout->setColumnStretch(col,1);

    emit plotAdded(plot);

    return plot;
}

void PlotMatrix::addRow()
{
    if( _num_rows==0 && _num_cols==0 )
    {
        addPlotWidget( 0, 0 );
        _num_rows = 1;
        _num_cols = 1;
    }
    else{
        for ( unsigned col = 0; col < colsCount(); col++ ) {
            addPlotWidget( _num_rows, col );
        }
        _num_rows++;
    }

    updateLayout();
}

void PlotMatrix::addColumn()
{
    if( _num_rows==0 && _num_cols==0 )
    {
        addPlotWidget( 0, 0 );
        _num_rows = 1;
        _num_cols = 1;
    }
    else {
        for ( unsigned row = 0; row < rowsCount(); row++ )
        {
            addPlotWidget( row, _num_cols );
        }
        _num_cols++;
    }
    updateLayout();
}

void PlotMatrix::swapPlots( unsigned rowA, unsigned colA, unsigned rowB, unsigned colB)
{
    QWidget *widgetA = _layout->itemAtPosition(rowA, colA)->widget();
    QWidget *widgetB = _layout->itemAtPosition(rowB, colB)->widget();

    _layout->removeItem( _layout->itemAtPosition(rowA, colA) );
    _layout->removeItem( _layout->itemAtPosition(rowB, colB) );

    _layout->addWidget(widgetA, rowB, colB);
    _layout->addWidget(widgetB, rowA, colA);
    updateLayout();
}

void PlotMatrix::removeColumn(unsigned column_to_delete)
{
    if(_num_rows==1 && _num_cols ==1 ) {
        return;
    }

    for(unsigned col = column_to_delete; col< _num_cols-1; col++)
    {
        for(unsigned row=0; row< _num_rows; row++)
        {
            this->swapPlots( row, col, row, col+1);
        }
    }
    for(unsigned row=0; row< _num_rows; row++)
    {
        plotAt( row, _num_cols -1)->close();
    }
    _layout->setColumnStretch(_num_cols -1,0);

    _num_cols--;
    if( _num_cols == 0){
        _num_rows = 0;
    }

    updateLayout();

}

void PlotMatrix::removeRow(unsigned row_to_delete)
{
    if(_num_rows==1 && _num_cols ==1 ) {
        return;
    }
    for(unsigned row = row_to_delete; row< _num_rows-1; row++)
    {
        for(unsigned col = 0; col< _num_cols; col++)
        {
            this->swapPlots( row, col, row+1, col);
        }
    }
    for(unsigned col=0; col< _num_cols; col++)
    {
        plotAt( _num_rows-1, col)->close();
    }
    _layout->setRowStretch(_num_rows -1,0);

    _num_rows--;
    if( _num_rows == 0){
        _num_cols = 0;
    }

    updateLayout();
}


PlotMatrix::~PlotMatrix(){}

unsigned PlotMatrix::rowsCount() const
{
    return _num_rows;
}

unsigned PlotMatrix::colsCount() const
{
    return _num_cols;
}

unsigned PlotMatrix::plotCount() const
{
    return _num_rows*_num_cols;
}

bool PlotMatrix::isColumnEmpty( unsigned col ) const
{
    for (int r=0; r < _layout->rowCount(); r++)
    {
        auto plot = plotAt(r, col);
        if( plot && ! plot->isEmpty() )  {
            return false;
        }
    }
    return true;
}

bool PlotMatrix::isRowEmpty(unsigned row ) const
{
    for (int c=0; c< _layout->columnCount(); c++)
    {
        auto plot = plotAt(row, c);
        if( plot && ! plot->isEmpty() )  {
            return false;
        }
    }
    return true;
}


PlotWidget* PlotMatrix::plotAt( unsigned row, unsigned column )
{
    QLayoutItem* item = _layout->itemAtPosition(row,column);
    if(item) {
        PlotWidget* plot = static_cast<PlotWidget*>( item->widget() );
        return plot;
    }
    return NULL;
}

const PlotWidget* PlotMatrix::plotAt( unsigned row, unsigned column ) const
{
    QLayoutItem* item = _layout->itemAtPosition(row,column);
    if(item) {
        PlotWidget* plot = static_cast<PlotWidget*>( item->widget() );
        return plot;
    }
    return NULL;
}

PlotWidget* PlotMatrix::plotAt( unsigned index )
{
  return plotAt(index% rowsCount(), index/rowsCount());
}

const PlotWidget* PlotMatrix::plotAt( unsigned index ) const
{
   return plotAt(index% rowsCount(), index/rowsCount());
}


void PlotMatrix::setAxisScale(QwtPlot::Axis axisId, unsigned row, unsigned col,
                               double min, double max, double step )
{
    PlotWidget *plt = plotAt( row, col );
    if ( plt )
    {
        plt->setAxisScale( axisId, min, max, step );
        plt->updateAxes();
    }
}

QDomElement PlotMatrix::xmlSaveState( QDomDocument &doc ) const
{
    QDomElement element = doc.createElement("plotmatrix");

    element.setAttribute("rows", _num_rows );
    element.setAttribute("columns", _num_cols );

    for(unsigned col = 0; col< _num_cols; col++)
    {
        for(unsigned row = 0; row< _num_rows; row++)
        {
            const PlotWidget* plot = plotAt(row,col);
            QDomElement child = plot->xmlSaveState(doc);

            child.setAttribute("row", row);
            child.setAttribute("col", col);

            element.appendChild( child );
        }
    }
    return element;
}

bool PlotMatrix::xmlLoadState( QDomElement &plotmatrix )
{
    if( !plotmatrix.hasAttribute("rows") || !plotmatrix.hasAttribute("columns") )
    {
        qWarning() << "No [rows] or [columns] attribute in <plotmatrix> XML file!";
        return false;
    }
    unsigned rows = plotmatrix.attribute("rows").toUInt();
    unsigned cols = plotmatrix.attribute("columns" ).toUInt();

    while( rows > _num_rows){ addRow(); }
    while( rows < _num_rows){ removeRow( _num_rows-1 );  }

    while( cols > _num_cols){ addColumn();  }
    while( cols < _num_cols){ removeColumn( _num_cols-1 ); }

    QMessageBox::StandardButton load_answer = QMessageBox::Ok;

    QDomElement plot_element;
    for (  plot_element = plotmatrix.firstChildElement( "plot" )  ;
           !plot_element.isNull();
           plot_element = plot_element.nextSiblingElement( "plot" ) )
    {
        if( !plot_element.hasAttribute("row") || !plot_element.hasAttribute("col") )
        {
            qWarning() << "No [row] or [col] attribute in <plot> XML file!";
            return false;
        }
        unsigned row = plot_element.attribute("row").toUInt();
        unsigned col = plot_element.attribute("col").toUInt();

        bool success = plotAt(row,col)->xmlLoadState( plot_element, &load_answer ) ;
        if( !success )
        {
            return false;
        }
    }
    return true;
}



void PlotMatrix::updateLayout()
{
    for ( unsigned row = 0; row < rowsCount(); row++ )
    {
        alignAxes( row, QwtPlot::xBottom );
        alignScaleBorder( row, QwtPlot::yLeft );
    }

    for ( unsigned col = 0; col < colsCount(); col++ )
    {
        alignAxes( col, QwtPlot::yLeft );
        alignScaleBorder( col, QwtPlot::xBottom );
    }
}

void PlotMatrix::replot()
{
    for ( unsigned i = 0; i< plotCount(); i++ )
    {
        PlotWidget *plot = plotAt(i);
        plot->replot();
    }
}


void PlotMatrix::setHorizontalLink(bool linked)
{
    _horizontal_link = linked;
}


void PlotMatrix::setName(const QString &new_name)
{
    _name = new_name;
}

const QString &PlotMatrix::name() const
{
    return _name;
}

QGridLayout *PlotMatrix::gridLayout()
{
    return _layout;
}

void PlotMatrix::maximumZoomOutHorizontal()
{
    for ( unsigned i = 0; i< plotCount(); i++ )
    {
        PlotWidget *plot = plotAt(i);
        if( plot->isEmpty() == false)
        {
            plot->on_zoomOutHorizontal_triggered( false );
        }
    }
    replot();
}

void PlotMatrix::maximumZoomOutVertical()
{
    for ( unsigned i = 0; i < plotCount(); i++ )
    {
        PlotWidget *plot = plotAt(i);
        if( plot->isEmpty() == false)
        {
            plot->on_zoomOutVertical_triggered(false);
        }
    }
    replot();
}

void PlotMatrix::maximumZoomOut()
{
    for ( unsigned i = 0; i < plotCount(); i++ )
    {
        PlotWidget *plot = plotAt(i);
        if( plot->isEmpty() == false)
        {
            plot->zoomOut(false);
        }
    }
    replot();
}


void PlotMatrix::on_singlePlotScaleChanged(PlotWidget *modified_plot, QRectF new_range)
{
    if( _horizontal_link )
    {
        for ( unsigned i = 0; i< plotCount(); i++ )
        {
            PlotWidget *plot = plotAt(i);
            if( plot->isEmpty() == false &&
                    modified_plot != plot &&
                    plot->isXYPlot() == false)
            {
                QRectF bound_act = plot->currentBoundingRect();
                bound_act.setLeft( new_range.left() );
                bound_act.setRight( new_range.right() );
                plot->setScale( bound_act, false );
                plot->replot();
            }
        }
    }
    emit undoableChange();
}

void PlotMatrix::alignAxes( unsigned rowOrColumn, QwtPlot::Axis axisId )
{
    bool iterating_rows = ( axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight );
    const int COUNT = iterating_rows ? rowsCount() : colsCount();

    double maxExtent = 0;

    for ( unsigned i = 0; i < COUNT; i++ )
    {
      QwtPlot *p = iterating_rows? plotAt( i, rowOrColumn ) : plotAt( rowOrColumn, i );
      if ( p )
      {
        QwtScaleWidget *scaleWidget = p->axisWidget( axisId );

        QwtScaleDraw *sd = scaleWidget->scaleDraw();
        sd->setMinimumExtent( 0.0 );

        const double extent = sd->extent( scaleWidget->font() );
        if ( extent > maxExtent )
          maxExtent = extent;
      }
    }

    for ( unsigned i = 0; i < COUNT; i++ )
    {
      QwtPlot *p = iterating_rows? plotAt( i, rowOrColumn ) : plotAt( rowOrColumn, i );
      if ( p )
      {
        QwtScaleWidget *scaleWidget = p->axisWidget( axisId );
        scaleWidget->scaleDraw()->setMinimumExtent( maxExtent );
      }
    }
}

void PlotMatrix::alignScaleBorder(unsigned rowOrColumn, QwtPlot::Axis axisId )
{
    if ( axisId == QwtPlot::yLeft || axisId == QwtPlot::yRight )
    {
        for ( unsigned col = 0; col < colsCount(); col++ )
        {
            QwtPlot *p = plotAt( rowOrColumn, col );
            if ( p )
                p->axisWidget( axisId )->setMinBorderDist( 10, 10 );
        }
    }
    else if ( axisId == QwtPlot::xTop || axisId == QwtPlot::xBottom )
    {
        for ( unsigned row = 0; row < rowsCount(); row++ )
        {
            QwtPlot *p = plotAt( row, rowOrColumn );
            if ( p )
                p->axisWidget( axisId )->setMinBorderDist( 15, 15 );
        }
    }
}
