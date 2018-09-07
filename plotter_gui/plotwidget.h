#ifndef DragableWidget_H
#define DragableWidget_H

#include <map>
#include <QObject>
#include <QTextEdit>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_legend.h>
#include <deque>
#include <QMessageBox>
#include <QTime>
#include "plotmagnifier.h"
#include "plotzoomer.h"
#include <qwt_plot_panner.h>
#include <QDomDocument>
#include "timeseries_qwt.h"
#include "customtracker.h"
#include "axis_limits_dialog.h"
#include <qwt_plot_legenditem.h>

class PlotWidget : public QwtPlot
{
    Q_OBJECT

public:

    PlotWidget(PlotDataMapRef& datamap, QWidget *parent=0);
    virtual ~PlotWidget();

    bool isEmpty() const;

    const std::map<std::string, std::shared_ptr<QwtPlotCurve> > &curveList() const;

    QDomElement xmlSaveState(QDomDocument &doc) const;

    bool xmlLoadState(QDomElement &element);

    QRectF currentBoundingRect() const;

    PlotData::RangeTime getMaximumRangeX() const;

    PlotData::RangeValue getMaximumRangeY( PlotData::RangeTime range_X, bool absolute_time ) const;

    void setScale( QRectF rect, bool emit_signal );

    void reloadPlotData( );

    void changeAxisX(QString curve_name);

    bool isXYPlot() const;

    void changeBackgroundColor(QColor color);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;

signals:
    void swapWidgetsRequested(PlotWidget* source, PlotWidget* destination);
    void rectChanged(PlotWidget* self, QRectF rect );
    void undoableChange();
    void trackerMoved(QPointF pos);
    void curveListChanged();

public slots:

    void updateCurves();

    void replot() ;

    void detachAllCurves();

    void zoomOut(bool emit_signal);

    void on_zoomOutHorizontal_triggered(bool emit_signal = true);

    void on_zoomOutVertical_triggered(bool emit_signal = true);

    void removeCurve(const std::string &name);

    void activateLegent(bool activate);

    void activateGrid(bool activate);

    void configureTracker(CurveTracker::Parameter val);

    void enableTracker(bool enable);

    void setTrackerPosition(double abs_time);

    void on_changeTimeOffset(double offset);

private slots:

    void on_noTransform_triggered(bool checked );

    void on_1stDerivativeTransform_triggered(bool checked);

    void on_2ndDerivativeTransform_triggered(bool checked);

    void on_convertToXY_triggered(bool checked);

    void on_savePlotToFile();

    void on_editAxisLimits_triggered();

private slots:
    void launchRemoveCurveDialog();
    void canvasContextMenuTriggered(const QPoint &pos);
    void on_changeColorsDialog_triggered();
    void on_changeColor(QString curve_name, QColor new_color);
    void on_showPoints_triggered(bool checked);
    void on_externallyResized(const QRectF &new_rect);


private:
    std::map<std::string, std::shared_ptr<QwtPlotCurve> > _curve_list;
    std::map<std::string, QwtPlotMarker*> _point_marker;

    QAction *_action_removeCurve;
    QAction *_action_removeAllCurves;
    QAction *_action_changeColorsDialog;
    QAction *_action_showPoints;
    QAction *_action_zoomOutHorizontally;
    QAction *_action_zoomOutVertically;
    QAction *_action_noTransform;
    QAction *_action_1stDerivativeTransform;
    QAction *_action_2ndDerivativeTransform;
    QAction *_action_phaseXY;
    QAction *_action_saveToFile;
    QAction *_action_editLimits;

    PlotZoomer* _zoomer;
    PlotMagnifier* _magnifier;
    QwtPlotPanner* _panner;

    CurveTracker* _tracker;
    QwtPlotLegendItem* _legend;
    QwtPlotGrid* _grid;

    PlotDataMapRef& _mapped_data;
    TimeseriesQwt::Transform _current_transform;

    bool addCurve(const std::string &name);

    void buildActions();

    void buildLegend();

    bool IsPointOnXAxis(const QPoint &p);
    enum class PlotHoverMode
    {
        NONE,
        ON_CANVAS,
        ON_X_AXIS
    };
    void drawPlotHover(PlotHoverMode mode);
    PlotHoverMode m_currentPlotHoverMode = PlotHoverMode::NONE;

    int   _fps_counter;
    QTime _fps_timeStamp;
    bool _show_line_and_points;

    void setDefaultRangeX();

    const PlotData* _axisX = nullptr;

    double _time_offset;

    PlotData::RangeValue _custom_Y_limits;

    AxisLimitsDialog* _axis_limits_dialog;

};

#endif
