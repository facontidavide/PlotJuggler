#include <functional>
#include <stdio.h>
#include <set>
#include <numeric>
#include <qwt_plot_canvas.h>
#include <QCheckBox>
#include <QCommandLineParser>
#include <QDebug>
#include <QDesktopServices>
#include <QDomDocument>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QMovie>
#include <QPluginLoader>
#include <QPushButton>
#include <QScrollBar>
#include <QStringListModel>
#include <QStringRef>
#include <QThread>
#include <QSettings>
#include <QWindow>
#include <QElapsedTimer>
#include <QHeaderView>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "busydialog.h"
#include "busytaskdialog.h"
#include "filterablelistwidget.h"
#include "tabbedplotwidget.h"
#include "selectlistdialog.h"
#include "aboutdialog.h"
#include "PlotJuggler/plotdata.h"


MainWindow::MainWindow(const QCommandLineParser &commandline_parser, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _undo_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this),
    _redo_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this),
    _toggle_streaming(QKeySequence(Qt::CTRL + Qt::Key_Space), this),
    _minimize_view(Qt::Key_F10, this),
    _minimized(false),
    _current_streamer(nullptr),
    _disable_undo_logging(false),
    _tracker_param( CurveTracker::VALUE )
{
    QLocale::setDefault(QLocale::c()); // set as default

    _test_option = (commandline_parser.isSet("test"));

    _curvelist_widget = new FilterableListWidget(this);
    _streamer_signal_mapper = new QSignalMapper(this);

    ui->setupUi(this);

    if( commandline_parser.isSet("buffer_size"))
    {
        int buffer_size = std::max(10, commandline_parser.value("buffer_size").toInt() );
        ui->streamingSpinBox->setMaximum(buffer_size);
    }

    {
        QIcon icon(":/icons/resources/office_chart_line_stacked.png");
        if (!icon.isNull())
        {
            this->setWindowIcon(icon);
            QApplication::setWindowIcon(icon);
        }
    }

    connect( _curvelist_widget->getView()->verticalScrollBar(), &QScrollBar::sliderMoved,
             this, &MainWindow::updateLeftTableValues );

    connect( _curvelist_widget, &FilterableListWidget::hiddenItemsChanged,
             this, &MainWindow::updateLeftTableValues );

    connect(_curvelist_widget, &FilterableListWidget::deleteCurve,
            this, &MainWindow::deleteDataOfSingleCurve );

    connect(_curvelist_widget->getView()->verticalScrollBar(),
            &QScrollBar::valueChanged,
            this, &MainWindow::updateLeftTableValues );

    connect( ui->timeSlider, &RealSlider::realValueChanged,
             this, &MainWindow::onTimeSlider_valueChanged );

    _main_tabbed_widget = new TabbedPlotWidget("Main Window", this, NULL, _mapped_plot_data, this);

    ui->plottingLayout->insertWidget(0, _main_tabbed_widget,1);
    ui->leftLayout->addWidget( _curvelist_widget );

    ui->splitter->setCollapsible(0,true);
    ui->splitter->setStretchFactor(0,2);
    ui->splitter->setStretchFactor(1,6);

    connect( ui->splitter, SIGNAL(splitterMoved(int,int)), SLOT(onSplitterMoved(int,int)) );

    createActions();

    loadPlugins( QCoreApplication::applicationDirPath() );
    loadPlugins("/usr/local/PlotJuggler/plugins");

    _undo_timer.start();

    // save initial state
    onUndoableChange();

    _replot_timer = new QTimer(this);
    _replot_timer->setInterval(40);
    connect(_replot_timer, &QTimer::timeout, this, &MainWindow::updateDataAndReplot);

    ui->menuFile->setToolTipsVisible(true);
    ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->streamingLabel->setHidden(true);
    ui->streamingSpinBox->setHidden(true);

    this->setMenuBar(ui->menuBar);
    ui->menuBar->setNativeMenuBar(false);

    connect(_streamer_signal_mapper, SIGNAL(mapped(QString)),
            this, SLOT(onActionLoadStreamer(QString)) );

    ui->actionDeleteAllData->setEnabled( _test_option );
    ui->actionReloadPrevious->setEnabled( false );

    if( _test_option )
    {
        connect( ui->actionLoadDummyData, &QAction::triggered,
                 this, &MainWindow::buildDummyData );
        buildDummyData();
    }
    else{
        ui->actionLoadDummyData->setVisible(false);
    }

    bool file_loaded = false;
    if( commandline_parser.isSet("datafile"))
    {
        onActionLoadDataFileImpl( commandline_parser.value("datafile"), true);
        file_loaded = true;
    }
    if( commandline_parser.isSet("layout"))
    {
        onActionLoadLayoutFromFile( commandline_parser.value("layout"), file_loaded);
    }

    QSettings settings;
    restoreGeometry(settings.value("MainWindow.geometry").toByteArray());

    bool activate_grid = settings.value("MainWindow.activateGrid", false).toBool();
    ui->pushButtonActivateGrid->setChecked(activate_grid);

    int streaming_buffer_value = settings.value("MainWindow.streamingBufferValue", 5).toInt();
    ui->streamingSpinBox->setValue(streaming_buffer_value);

    bool datetime_display  = settings.value("MainWindow.dateTimeDisplay", false).toBool();
    ui->pushButtonUseDateTime->setChecked( datetime_display );

    ui->widgetOptions->setVisible( ui->pushButtonOptions->isChecked() );
    ui->line->setVisible( ui->pushButtonOptions->isChecked() );

    //----------------------------------------------------------
    QIcon trackerIconA, trackerIconB, trackerIconC;

    trackerIconA.addFile(QStringLiteral(":/icons/resources/line_tracker.png"), QSize(36, 36));
    trackerIconB.addFile(QStringLiteral(":/icons/resources/line_tracker_1.png"), QSize(36, 36));
    trackerIconC.addFile(QStringLiteral(":/icons/resources/line_tracker_a.png"), QSize(36, 36));

    _tracker_button_icons[CurveTracker::LINE_ONLY]  = trackerIconA;
    _tracker_button_icons[CurveTracker::VALUE]      = trackerIconB;
    _tracker_button_icons[CurveTracker::VALUE_NAME] = trackerIconC;

    int tracker_setting = settings.value("MainWindow.timeTrackerSetting", (int)CurveTracker::VALUE ).toInt();
    _tracker_param = static_cast<CurveTracker::Parameter>(tracker_setting);

    ui->pushButtonTimeTracker->setIcon( _tracker_button_icons[_tracker_param] );

    forEachWidget( [&](PlotWidget* plot) {
        plot->configureTracker(_tracker_param);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onUndoableChange()
{
    if(_disable_undo_logging) return;

    int elapsed_ms = _undo_timer.restart();

    // overwrite the previous
    if( elapsed_ms < 100)
    {
        if( _undo_states.empty() == false)
            _undo_states.pop_back();
    }

    while( _undo_states.size() >= 100 ) _undo_states.pop_front();
    _undo_states.push_back( xmlSaveState() );
    _redo_states.clear();
    //  qDebug() << "undo " << _undo_states.size();
}


void MainWindow::onRedoInvoked()
{
    _disable_undo_logging = true;
    if( _redo_states.size() > 0)
    {
        QDomDocument state_document = _redo_states.back();
        while( _undo_states.size() >= 100 ) _undo_states.pop_front();
        _undo_states.push_back( state_document );
        _redo_states.pop_back();

        xmlLoadState( state_document );
    }
    _disable_undo_logging = false;
}


void MainWindow::updateLeftTableValues()
{
    auto table_model = _curvelist_widget->getTable();
    auto table_view  = _curvelist_widget->getView();

    if( _curvelist_widget->is2ndColumnHidden() == false)
    {
        table_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        table_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);

        const int vertical_height = table_view->visibleRegion().boundingRect().height();

        for (int row = 0; row < _curvelist_widget->rowCount(); row++)
        {
            int vertical_pos = table_view->rowViewportPosition(row);
            if( vertical_pos < 0 || table_view->isRowHidden(row) ){ continue; }
            if( vertical_pos > vertical_height){ break; }

            const std::string& name = table_model->item(row,0)->text().toStdString();
            auto it = _mapped_plot_data.numeric.find(name);
            if( it !=  _mapped_plot_data.numeric.end())
            {
                auto& data = it->second;

                double num = 0.0;
                bool valid = false;

                if( _tracker_time < std::numeric_limits<double>::max())
                {
                    auto value = data.getYfromX( _tracker_time );
                    if(value){
                        valid = true;
                        num = value.value();
                    }
                }
                else{
                    if( data.size() > 0) {
                        valid = true;
                        num = data.back().y;
                    }
                }
                if( valid)
                {
                    QString num_text = QString::number( num, 'f', 3);
                    if(num_text.contains('.'))
                    {
                        int idx = num_text.length() -1;
                        while( num_text[idx] == '0' )
                        {
                            num_text[idx] = ' ';
                            idx--;
                        }
                        if(  num_text[idx] == '.') num_text[idx] = ' ';
                    }
                    table_model->item(row,1)->setText(num_text + ' ');
                }
            }
        }
        table_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    }
}


void MainWindow::onTrackerMovedFromWidget(QPointF relative_pos)
{
    _tracker_time = relative_pos.x() + _time_offset.get();

    auto prev = ui->timeSlider->blockSignals(true);
    ui->timeSlider->setRealValue( relative_pos.x() );
    ui->timeSlider->blockSignals(prev);

    onTrackerTimeUpdated( _tracker_time, true );
}

void MainWindow::onTimeSlider_valueChanged(double relative_time)
{
    _tracker_time = relative_time + _time_offset.get();
    onTrackerTimeUpdated( _tracker_time, true );
}

void MainWindow::onTrackerTimeUpdated(double absolute_time, bool do_replot)
{
    updatedDisplayTime();
    updateLeftTableValues();

    for ( auto& it: _state_publisher)
    {
        it.second->updateState( absolute_time);
    }

    forEachWidget( [&](PlotWidget* plot)
    {
        plot->setTrackerPosition( _tracker_time );
        if(do_replot)
        {
            plot->replot();
        }
    } );
}

void MainWindow::createTabbedDialog(QString suggest_win_name, PlotMatrix* first_tab)
{
    if( suggest_win_name.isEmpty())
    {
        for (int i=0; i<= TabbedPlotWidget::instances().size(); i++)
        {
            suggest_win_name = QString("Window%1").arg(i);
            TabbedPlotWidget* tw = TabbedPlotWidget::instance(suggest_win_name);
            if( tw == nullptr )
            {
                break;
            }
        }
    }

    SubWindow* window = new SubWindow(suggest_win_name, first_tab, _mapped_plot_data, this );

    connect( window, SIGNAL(destroyed(QObject*)),  this,  SLOT(onFloatingWindowDestroyed(QObject*)) );
    connect( window, SIGNAL(destroyed(QObject*)),  this,  SLOT(onUndoableChange()) );

    window->tabbedWidget()->setStreamingMode( isStreamingActive() );

    window->setAttribute( Qt::WA_DeleteOnClose, true );
    window->show();
    window->activateWindow();
    window->raise();

    if (this->signalsBlocked() == false) onUndoableChange();
}


void MainWindow::createActions()
{
    _undo_shortcut.setContext(Qt::ApplicationShortcut);
    _redo_shortcut.setContext(Qt::ApplicationShortcut);
    _minimize_view.setContext(Qt::ApplicationShortcut);

    connect( &_undo_shortcut, &QShortcut::activated, this, &MainWindow::onUndoInvoked );
    connect( &_redo_shortcut, &QShortcut::activated, this, &MainWindow::onRedoInvoked );
    connect( &_minimize_view, &QShortcut::activated, this, &MainWindow::on_minimizeView);
    connect( &_toggle_streaming, &QShortcut::activated, this, &MainWindow::on_ToggleStreaming );

    connect( ui->actionMaximizePlots, &QAction::triggered, this, &MainWindow::on_minimizeView);

    QShortcut* open_menu_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_F), this);
    connect( open_menu_shortcut, &QShortcut::activated, [this](){
        ui->menuFile->exec( ui->menuBar->mapToGlobal(QPoint(0,25)));
    } );

    QShortcut* open_streaming_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_S), this);
    connect( open_streaming_shortcut, &QShortcut::activated, [this](){
        ui->menuStreaming->exec( ui->menuBar->mapToGlobal(QPoint(50,25)));
    } );

    QShortcut* open_publish_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_P), this);
    connect( open_publish_shortcut, &QShortcut::activated, [this](){
        ui->menuPublishers->exec( ui->menuBar->mapToGlobal(QPoint(140,25)));
    } );

    QShortcut* open_help_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_H), this);
    connect( open_help_shortcut, &QShortcut::activated, [this](){
        ui->menuHelp->exec( ui->menuBar->mapToGlobal(QPoint(230,25)));
    } );

    //---------------------------------------------

    connect(ui->actionSaveLayout, &QAction::triggered,         this, &MainWindow::onActionSaveLayout );
    connect(ui->actionLoadLayout, &QAction::triggered,         this, &MainWindow::onActionLoadLayout );
    connect(ui->actionLoadData, &QAction::triggered,           this, &MainWindow::onActionLoadDataFile );
    connect(ui->actionLoadRecentDatafile, &QAction::triggered, this, &MainWindow::onActionReloadRecentDataFile );
    connect(ui->actionLoadRecentLayout, &QAction::triggered,   this, &MainWindow::onActionReloadRecentLayout );
    connect(ui->actionDeleteAllData, &QAction::triggered,      this, &MainWindow::onDeleteLoadedData );

    connect(ui->actionReloadPrevious, &QAction::triggered,     this, &MainWindow::onReloadDatafile );

    //---------------------------------------------

    QSettings settings;
    if( settings.contains("MainWindow.recentlyLoadedDatafile") )
    {
        QString filename = settings.value("MainWindow.recentlyLoadedDatafile").toString();
        ui->actionLoadRecentDatafile->setText( "Load data from: " + filename);
        ui->actionLoadRecentDatafile->setEnabled( true );
    }
    else{
        ui->actionLoadRecentDatafile->setEnabled( false );
    }

    if( settings.contains("MainWindow.recentlyLoadedLayout") )
    {
        QString filename = settings.value("MainWindow.recentlyLoadedLayout").toString();
        ui->actionLoadRecentLayout->setText( "Load layout from: " + filename);
        ui->actionLoadRecentLayout->setEnabled( true );
    }
    else{
        ui->actionLoadRecentLayout->setEnabled( false );
    }
}

void MainWindow::loadPlugins(QString directory_name)
{
    static std::set<QString> loaded_plugins;

    QDir pluginsDir( directory_name );

    for (QString filename: pluginsDir.entryList(QDir::Files))
    {
        QFileInfo fileinfo(filename);
        if( fileinfo.suffix() != "so" && fileinfo.suffix() != "dll" && fileinfo.suffix() != "dylib"){
            continue;
        }

        if( loaded_plugins.find( filename ) != loaded_plugins.end())
        {
            continue;
        }

        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(filename), this);

        QObject *plugin = pluginLoader.instance();
        if (plugin)
        {
            DataLoader *loader        = qobject_cast<DataLoader *>(plugin);
            StatePublisher *publisher = qobject_cast<StatePublisher *>(plugin);
            DataStreamer *streamer    =  qobject_cast<DataStreamer *>(plugin);

            QString plugin_name;
            if( loader )    plugin_name = loader->name();
            if( publisher ) plugin_name = publisher->name();
            if( streamer )  plugin_name = streamer->name();
            plugin_name.replace(" ", "_");

            if( loaded_plugins.find(plugin_name) == loaded_plugins.end())
            {
                loaded_plugins.insert( plugin_name );
            }
            else{
                QMessageBox::warning(0, tr("Warning"),
                                     tr("Trying to load twice a plugin with name [%1].\n"
                                        "Only the first will be loaded.").arg(plugin_name) );
                continue;
            }

            if (loader)
            {
                qDebug() << filename << ": is a DataLoader plugin";
                if( !_test_option && loader->isDebugPlugin())
                {
                    qDebug() << filename << "...but will be ignored unless the argument -t is used.";
                }
                else{
                    _data_loader.insert( std::make_pair( plugin_name, loader) );
                }
            }
            else if (publisher)
            {
                publisher->setDataMap( &_mapped_plot_data );
                qDebug() << filename << ": is a StatePublisher plugin";
                if( !_test_option && publisher->isDebugPlugin())
                {
                    qDebug() << filename << "...but will be ignored unless the argument -t is used.";
                }
                else
                {
                    _state_publisher.insert( std::make_pair(plugin_name, publisher) );

                    QAction* activatePublisher = new QAction(tr("Start: ") + plugin_name , this);
                    activatePublisher->setCheckable(true);
                    activatePublisher->setChecked(false);
                    ui->menuPublishers->setEnabled(true);
                    ui->menuPublishers->addAction(activatePublisher);

                    publisher->setParentMenu( ui->menuPublishers );

                    ui->menuPublishers->addSeparator();

                    connect(activatePublisher, &QAction::toggled,
                            [=](bool enable) { publisher->setEnabled( enable ); } );

                }
            }
            else if (streamer)
            {
                qDebug() << filename << ": is a DataStreamer plugin";
                if( !_test_option && streamer->isDebugPlugin())
                {
                    qDebug() << filename << "...but will be ignored unless the argument -t is used.";
                }
                else{
                    _data_streamer.insert( std::make_pair(plugin_name , streamer ) );

                    QAction* startStreamer = new QAction(QString("Start: ") + plugin_name, this);
                    ui->menuStreaming->setEnabled(true);
                    ui->menuStreaming->addAction(startStreamer);

                    streamer->setParentMenu( ui->menuStreaming );
                    ui->menuStreaming->addSeparator();

                    connect(startStreamer, SIGNAL(triggered()), _streamer_signal_mapper, SLOT(map()) );
                    _streamer_signal_mapper->setMapping(startStreamer, plugin_name );

                    connect(streamer, &DataStreamer::connectionClosed,
                            ui->actionStopStreaming, &QAction::trigger );
                }
            }
        }
        else{
            if( pluginLoader.errorString().contains("is not an ELF object") == false)
            {
                qDebug() << filename << ": " << pluginLoader.errorString();
            }
        }
    }
}

void MainWindow::buildDummyData()
{
    PlotDataMapRef datamap;

    static int count = 0;
    size_t SIZE = 10000;
    QElapsedTimer timer;
    timer.start();
    QStringList  words_list;
    words_list << "world/siam" << "world/tre" << "walk/piccoli" << "walk/porcellin"
               << "fly/high/mai" << "fly/high/nessun" << "fly/low/ci" << "fly/low/dividera"
               << "data_1" << "data_2" << "data_3" << "data_10";

    for( int i=0; i<10; i++)
    {
        words_list.append(QString("data_vect/%1").arg(count++));
    }

    for( const QString& name: words_list)
    {
        double A =  6* ((double)qrand()/(double)RAND_MAX)  - 3;
        double B =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double C =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double D =  20* ((double)qrand()/(double)RAND_MAX)  ;

        auto it = datamap.addNumeric( name.toStdString() );
        PlotData& plot = it->second;

        double t = 0;
        for (unsigned indx=0; indx<SIZE; indx++)
        {
            t += 0.01;
            plot.pushBack( PlotData::Point( t,  A*sin(B*t + C) + D*t*0.02 ) ) ;
        }
    }

    PlotData& sin_plot =  datamap.addNumeric( "_sin" )->second;
    PlotData& cos_plot =  datamap.addNumeric( "_cos" )->second;

    double t = 0;
    for (unsigned indx=0; indx<SIZE; indx++)
    {
        t += 0.01;
        sin_plot.pushBack( PlotData::Point( t,  1.0*sin(t*0.4) ) ) ;
        cos_plot.pushBack( PlotData::Point( t,  2.0*cos(t*0.4) ) ) ;
    }

    importPlotDataMap(datamap,true);
}

void MainWindow::onSplitterMoved(int , int )
{
    QList<int> sizes = ui->splitter->sizes();
    int maxLeftWidth = _curvelist_widget->maximumWidth();
    int totalWidth = sizes[0] + sizes[1];

    if( sizes[0] > maxLeftWidth)
    {
        sizes[0] = maxLeftWidth;
        sizes[1] = totalWidth - maxLeftWidth;
        ui->splitter->setSizes(sizes);
    }
}

void MainWindow::resizeEvent(QResizeEvent *)
{
    onSplitterMoved( 0, 0 );
}


void MainWindow::onPlotAdded(PlotWidget* plot)
{
    connect( plot, &PlotWidget::undoableChange,
             this, &MainWindow::onUndoableChange );

    connect( plot, &PlotWidget::trackerMoved,
             this, &MainWindow::onTrackerMovedFromWidget);

    connect( plot, &PlotWidget::swapWidgetsRequested,
             this, &MainWindow::onSwapPlots);

    connect( this, &MainWindow::requestRemoveCurveByName,
             plot, &PlotWidget::removeCurve) ;

    connect( plot, &PlotWidget::curveListChanged,
             this, &MainWindow::updateTimeSlider) ;

    connect( plot, &PlotWidget::curveListChanged,
             this, &MainWindow::updateTimeOffset) ;

    connect( &_time_offset, SIGNAL( valueChanged(double)),
             plot, SLOT(on_changeTimeOffset(double)) );

    plot->on_changeTimeOffset( _time_offset.get() );
    plot->activateGrid( ui->pushButtonActivateGrid->isChecked() );
    plot->enableTracker( !isStreamingActive() );
    plot->configureTracker( _tracker_param );
}

void MainWindow::onPlotMatrixAdded(PlotMatrix* matrix)
{
    connect( matrix, &PlotMatrix::plotAdded,      this, &MainWindow:: onPlotAdded);
    connect( matrix, &PlotMatrix::undoableChange, this, &MainWindow:: onUndoableChange );
}

QDomDocument MainWindow::xmlSaveState() const
{
    QDomDocument doc;
    QDomProcessingInstruction instr =
            doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");

    doc.appendChild(instr);

    QDomElement root = doc.createElement( "root" );

    for (auto& it: TabbedPlotWidget::instances() )
    {
        QDomElement tabbed_area = it.second->xmlSaveState(doc);
        root.appendChild( tabbed_area );
    }

    doc.appendChild(root);

    QDomElement relative_time = doc.createElement( "use_relative_time_offset" );
    relative_time.setAttribute("enabled", ui->pushButtonRemoveTimeOffset->isChecked() );
    root.appendChild( relative_time );

    return doc;
}

void MainWindow::checkAllCurvesFromLayout(const QDomElement& root)
{
    std::set<std::string> curves;
    // ugly code, I am sorry
    for ( QDomElement   tw = root.firstChildElement(  "tabbed_widget" )  ;
          !tw.isNull(); tw = tw.nextSiblingElement( "tabbed_widget" ) )
    {
        for ( QDomElement   pm = tw.firstChildElement(  "plotmatrix" )  ;
              !pm.isNull(); pm = pm.nextSiblingElement( "plotmatrix" ) )
        {
            for ( QDomElement   pl = pm.firstChildElement(  "plot" )  ;
                  !pl.isNull(); pl = pl.nextSiblingElement( "plot" ) )
            {
                for ( QDomElement   cv = pl.firstChildElement(  "curve" )  ;
                      !cv.isNull(); cv = cv.nextSiblingElement( "curve" ) )
                {
                    curves.insert( cv.attribute("name").toStdString() );
                }
            }
        }
    }

    std::vector<std::string> missing_curves;

    for (auto& curve_name: curves)
    {
        if( _mapped_plot_data.numeric.count( curve_name ) == 0)
        {
            missing_curves.push_back(curve_name);
        }
    }
    if( missing_curves.size() > 0 )
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Warning");
        msgBox.setText(tr("One or more timeseries in the layout haven't been loaded yet\n"
                          "What do you want to do?"));

        QPushButton* buttonRemove = msgBox.addButton(tr("Remove curves from plots"), QMessageBox::RejectRole);
        QPushButton* buttonPlaceholder = msgBox.addButton(tr("Create empty placeholders"), QMessageBox::YesRole);
        msgBox.setDefaultButton(buttonPlaceholder);
        msgBox.exec();
        if( msgBox.clickedButton() == buttonPlaceholder )
        {
            for(auto& name: missing_curves )
            {
                _curvelist_widget->addItem( QString::fromStdString( name ) );
                _mapped_plot_data.addNumeric(name);
            }
            _curvelist_widget->refreshColumns();
        }
    }
}

bool MainWindow::xmlLoadState(QDomDocument state_document)
{
    QDomElement root = state_document.namedItem("root").toElement();
    if ( root.isNull() ) {
        qWarning() << "No <root> element found at the top-level of the XML file!";
        return false;
    }

    size_t num_floating = 0;
    std::map<QString,QDomElement> tabbed_widgets_with_name;

    for (QDomElement tw = root.firstChildElement(  "tabbed_widget" )  ;
         tw.isNull() == false;
         tw = tw.nextSiblingElement( "tabbed_widget" ) )
    {
        if( tw.attribute("parent") != ("main_window") )
        {
            num_floating++;
        }
        tabbed_widgets_with_name[ tw.attribute("name") ] = tw;
    }

    // add if missing
    for(const auto& it: tabbed_widgets_with_name)
    {
        if( TabbedPlotWidget::instance( it.first ) == nullptr)
        {
            createTabbedDialog( it.first, NULL );
        }
    }

    // remove those which don't share list of names
    for(const auto& it: TabbedPlotWidget::instances())
    {
        if( tabbed_widgets_with_name.count( it.first ) == 0)
        {
            it.second->deleteLater();
        }
    }

    //-----------------------------------------------------
    checkAllCurvesFromLayout(root);
    //-----------------------------------------------------

    for ( QDomElement tw = root.firstChildElement(  "tabbed_widget" )  ;
          tw.isNull() == false;
          tw = tw.nextSiblingElement( "tabbed_widget" ) )
    {
        TabbedPlotWidget* tabwidget = TabbedPlotWidget::instance( tw.attribute("name"));
        tabwidget->xmlLoadState( tw );
    }

    QDomElement relative_time = root.firstChildElement( "use_relative_time_offset" );
    if( !relative_time.isNull())
    {
        bool remove_offset = (relative_time.attribute("enabled") == QString("1"));
        ui->pushButtonRemoveTimeOffset->setChecked(remove_offset);
    }
    return true;
}

void MainWindow::onActionSaveLayout()
{
    QDomDocument doc = xmlSaveState();

    //--------------------------
    savePluginState(doc);
    //--------------------------

    QDomElement root = doc.namedItem("root").toElement();

    if( !_loaded_datafile.isEmpty() || _current_streamer )
    {
        auto reply = QMessageBox::question(0, tr("Hey!"),
                                           tr("Do you want the layout to remember the source of your data,\n"
                                              "i.e. the Datafile used or the Streaming Plugin loaded ?"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
        if( reply == QMessageBox::Yes )
        {
            if( _loaded_datafile.isEmpty() == false)
            {
                QDomElement previously_loaded_datafile =  doc.createElement( "previouslyLoadedDatafile" );
                previously_loaded_datafile.setAttribute("filename", _loaded_datafile );
                root.appendChild( previously_loaded_datafile );
            }
        }
        if( _current_streamer )
        {
            QDomElement loaded_streamer =  doc.createElement( "previouslyLoadedStreamer" );
            QString streamer_name = _current_streamer->name();
            streamer_name.replace(" ", "_");
            loaded_streamer.setAttribute("name", streamer_name );
            root.appendChild( loaded_streamer );
        }
    }
    //------------------------------------

    QSettings settings;

    QString directory_path  = settings.value("MainWindow.lastLayoutDirectory",
                                             QDir::currentPath() ).toString();

    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("xml");
    saveDialog.setNameFilter("XML (*.xml)");
    saveDialog.setDirectory(directory_path);
    saveDialog.exec();

    if(saveDialog.result() != QDialog::Accepted || saveDialog.selectedFiles().empty())
    {
        return;
    }

    QString fileName = saveDialog.selectedFiles().first();

    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << doc.toString() << endl;
    }
}

void MainWindow::deleteDataOfSingleCurve(const std::string& curve_name)
{
    auto plot_curve = _mapped_plot_data.numeric.find( curve_name );
    if( plot_curve == _mapped_plot_data.numeric.end())
    {
        return;
    }

    emit requestRemoveCurveByName( curve_name );
    _mapped_plot_data.numeric.erase( plot_curve );

    int row = _curvelist_widget->findRowByName( curve_name );
    if( row != -1 )
    {
        _curvelist_widget->removeRow(row);
    }

    if( _curvelist_widget->rowCount() == 0)
    {
        ui->actionDeleteAllData->setEnabled( false );
    }
}

void MainWindow::deleteDataMultipleCurves(const std::vector<std::string> &curves_name)
{
    for( auto& name: curves_name)
    {
        _mapped_plot_data.numeric.erase(name);
        emit requestRemoveCurveByName( name );
    }
    // it is much faster in many case to rebuild everything from scratch
    _curvelist_widget->clear();

    for( auto& it: _mapped_plot_data.numeric)
    {
        _curvelist_widget->addItem( QString::fromStdString(it.first) );
    }

    if( _curvelist_widget->rowCount() == 0)
    {
        ui->actionDeleteAllData->setEnabled( false );
    }
}


void MainWindow::onDeleteLoadedData()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("Warning");
    msgBox.setText(tr("Do you really want to REMOVE the loaded data?\n"));
    msgBox.addButton(QMessageBox::No);
    msgBox.addButton(QMessageBox::Yes);
    msgBox.setDefaultButton(QMessageBox::Yes);
    QPushButton* buttonPlaceholder = msgBox.addButton(tr("Keep empty placeholders"), QMessageBox::NoRole);
    auto reply = msgBox.exec();

    if( reply == QMessageBox::No ) {
        return;
    }

    if( msgBox.clickedButton() == buttonPlaceholder )
    {
        for( auto& it: _mapped_plot_data.numeric )
        {
            emit requestRemoveCurveByName( it.first );
            it.second.clear();
        }
        for( auto& it: _mapped_plot_data.user_defined )
        {
            it.second.clear();
        }

        for(const auto& it: TabbedPlotWidget::instances())
        {
            PlotMatrix* matrix =  it.second->currentTab() ;
            matrix->maximumZoomOut(); // includes replot
        }
    }
    else
    {
        forEachWidget( [](PlotWidget* plot) {
            plot->detachAllCurves();
        } );
        _mapped_plot_data.numeric.clear();
        _mapped_plot_data.user_defined.clear();

        _curvelist_widget->clear();

        ui->actionDeleteAllData->setEnabled( false );
    }
}

void MainWindow::onActionLoadDataFile()
{
    if( _data_loader.empty())
    {
        QMessageBox::warning(0, tr("Warning"),
                             tr("No plugin was loaded to process a data file\n") );
        return;
    }

    QSettings settings;

    QString file_extension_filter;

    std::set<QString> extensions;

    for (auto& it: _data_loader)
    {
        DataLoader* loader = it.second;
        for (QString extension: loader->compatibleFileExtensions() )
        {
            extensions.insert( extension.toLower() );
        }
    }

    for (const auto& it: extensions)
    {
        file_extension_filter.append( QString(" *.") + it );
    }

    QString directory_path = settings.value("MainWindow.lastDatafileDirectory", QDir::currentPath() ).toString();

    QString filename = QFileDialog::getOpenFileName(this, "Open Datafile",
                                                    directory_path,
                                                    file_extension_filter);
    if (filename.isEmpty()) {
        return;
    }

    directory_path = QFileInfo(filename).absolutePath();

    settings.setValue("MainWindow.lastDatafileDirectory", directory_path);
    settings.setValue("MainWindow.recentlyLoadedDatafile", filename);

    ui->actionLoadRecentDatafile->setText("Load data from: " + filename);

    onActionLoadDataFileImpl(filename, false );
}

void MainWindow::onReloadDatafile()
{
    QSettings settings;
    if( settings.contains("MainWindow.recentlyLoadedDatafile") )
    {
        QString filename = settings.value("MainWindow.recentlyLoadedDatafile").toString();
        onActionLoadDataFileImpl(filename, true);
    }
}

void MainWindow::onActionReloadRecentDataFile()
{
    QSettings settings;
    if( settings.contains("MainWindow.recentlyLoadedDatafile") )
    {
        QString filename = settings.value("MainWindow.recentlyLoadedDatafile").toString();
        onActionLoadDataFileImpl(filename, false);
    }
}

template <typename T>
void importPlotDataMapHelper(std::unordered_map<std::string,T>& source,
                             std::unordered_map<std::string,T>& destination,
                             bool delete_older)
{
    for (auto& it: source)
    {
        const std::string& name  = it.first;
        T& source_plot  = it.second;
        auto plot_with_same_name = destination.find(name);

        // this is a new plot
        if( plot_with_same_name == destination.end() )
        {
            plot_with_same_name = destination.emplace( std::piecewise_construct,
                                                       std::forward_as_tuple(name),
                                                       std::forward_as_tuple(name)
                                                       ).first;
        }
        T& destination_plot = plot_with_same_name->second;
        if( delete_older )
        {
            double max_range_x = destination_plot.maximumRangeX();
            destination_plot.swapData(source_plot);
            destination_plot.setMaximumRangeX(max_range_x); // just in case
        }
        else
        {
            for (size_t i=0; i< source_plot.size(); i++)
            {
                destination_plot.pushBack( source_plot.at(i) );
            }
        }
        source_plot.clear();
    }
}

void MainWindow::importPlotDataMap(PlotDataMapRef& new_data, bool delete_older)
{
    if( new_data.user_defined.empty() && new_data.numeric.empty() )
    {
        return;
    }

    bool curvelist_modified = false;
    for (auto& it: new_data.numeric)
    {
        const std::string& name  = it.first;
        if( _mapped_plot_data.numeric.count(name) == 0)
        {
            _curvelist_widget->addItem( QString::fromStdString( name ) );
            curvelist_modified = true;
        }
    }

    //---------------------------------------------
    importPlotDataMapHelper( new_data.user_defined, _mapped_plot_data.user_defined, delete_older );
    importPlotDataMapHelper( new_data.numeric, _mapped_plot_data.numeric, delete_older );
    //---------------------------------------------

    if( delete_older && _mapped_plot_data.numeric.size() > new_data.numeric.size() )
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(0, tr("Warning"),
                                      tr("Do you want to remove the previously loaded data?\n"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes );
        if( reply == QMessageBox::Yes )
        {
            std::vector<std::string> data_to_remove;

            for (auto& it: _mapped_plot_data.numeric )
            {
                auto& name = it.first;
                if( new_data.numeric.find( name ) == new_data.numeric.end() ){
                    data_to_remove.push_back(name);
                }
            }
            this->deleteDataMultipleCurves( data_to_remove );
        }
    }

    if( delete_older && _mapped_plot_data.user_defined.size() > new_data.user_defined.size() )
    {
        std::vector<std::string> data_to_remove;

        for (auto& it: _mapped_plot_data.user_defined )
        {
            auto& name = it.first;
            if( new_data.user_defined.find( name ) == new_data.user_defined.end() ){
                data_to_remove.push_back(name);
            }
        }
        for (auto& to_remove: data_to_remove )
        {
            _mapped_plot_data.user_defined.erase( to_remove );
        }
    }

    if( curvelist_modified )
    {
        _curvelist_widget->refreshColumns();
    }
}

bool MainWindow::isStreamingActive() const
{
    return ui->pushButtonStreaming->isChecked() && _current_streamer;
}

void MainWindow::onActionLoadDataFileImpl(QString filename, bool reuse_last_configuration )
{
    const QString extension = QFileInfo(filename).suffix().toLower();

    typedef std::map<QString,DataLoader*>::iterator MapIterator;

    std::vector<MapIterator> compatible_loaders;

    for (MapIterator it = _data_loader.begin(); it != _data_loader.end(); it++)
    {
        DataLoader* data_loader = it->second;
        std::vector<const char*> extensions = data_loader->compatibleFileExtensions();

        for(auto& ext: extensions){

            if( extension == QString(ext).toLower()){
                compatible_loaders.push_back( it );
                break;
            }
        }
    }

    _last_dataloader = nullptr;

    if( compatible_loaders.size() == 1)
    {
        _last_dataloader = compatible_loaders.front()->second;
    }
    else{
        static QString last_plugin_name_used;

        QStringList names;
        for (auto& cl: compatible_loaders)
        {
            const auto& name = cl->first;

            if( name == last_plugin_name_used ){
                names.push_front( name );
            }
            else{
                names.push_back( name );
            }
        }

        bool ok;
        QString plugin_name = QInputDialog::getItem(this, tr("QInputDialog::getItem()"),
                                                    tr("Select the loader to use:"),
                                                    names, 0, false, &ok);
        if (ok && !plugin_name.isEmpty())
        {
            _last_dataloader = _data_loader[ plugin_name ];
            last_plugin_name_used = plugin_name;
        }
    }

    if( _last_dataloader )
    {
        QFile file(filename);

        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(this, tr("Datafile"),
                                 tr("Cannot read file %1:\n%2.")
                                 .arg(filename)
                                 .arg(file.errorString()));
            return;
        }
        file.close();

        _loaded_datafile = filename;
        ui->actionDeleteAllData->setEnabled( true );
        ui->actionReloadPrevious->setEnabled( true );

        try{
            PlotDataMapRef mapped_data = _last_dataloader->readDataFromFile( filename, reuse_last_configuration );
            importPlotDataMap(mapped_data, true);
        }
        catch(std::exception &ex)
        {
            QMessageBox::warning(this, tr("Exception from the plugin"),
                                 tr("The plugin [%1] thrown the following exception: \n\n %3\n")
                                 .arg(_last_dataloader->name()).arg(ex.what()) );
            return;
        }
    }
    else{
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot read files with extension %1.\n No plugin can handle that!\n")
                             .arg(filename) );
    }
    _curvelist_widget->updateFilter();
    updateDataAndReplot();

}

void MainWindow::onActionReloadRecentLayout()
{
    onActionLoadLayout( true );
}

void MainWindow::onActionLoadStreamer(QString streamer_name)
{
    if( _current_streamer )
    {
        _current_streamer->shutdown();
        _current_streamer = nullptr;
    }

    if( _data_streamer.empty())
    {
        qDebug() << "Error, no streamer loaded";
        return;
    }

    if( _data_streamer.size() == 1)
    {
        _current_streamer = _data_streamer.begin()->second;
    }
    else if( _data_streamer.size() > 1)
    {
        auto it = _data_streamer.find(streamer_name);
        if( it != _data_streamer.end())
        {
            _current_streamer = it->second;
        }
        else{
            qDebug() << "Error. The streamer " << streamer_name <<
                        " can't be loaded";
            return;
        }
    }

    bool started = false;
    try{
        started = _current_streamer && _current_streamer->start();
    }
    catch(std::runtime_error& err)
    {
        QMessageBox::warning(this, tr("Exception from the plugin"),
                             tr("The plugin thrown the following exception: \n\n %1\n")
                             .arg(err.what()) );
        return;
    }
    if( started )
    {
        {
            std::lock_guard<std::mutex> lock( _current_streamer->mutex() );
            importPlotDataMap( _current_streamer->dataMap(), true );
        }

        for(auto& action: ui->menuStreaming->actions()) {
            action->setEnabled(false);
        }
        ui->actionClearBuffer->setEnabled(true);

        ui->actionStopStreaming->setEnabled(true);
        ui->actionDeleteAllData->setEnabled( false );
        ui->actionDeleteAllData->setToolTip("Stop streaming to be able to delete the data");
        ui->actionReloadPrevious->setEnabled( false );

        ui->pushButtonStreaming->setEnabled(true);
        ui->pushButtonStreaming->setChecked(true);
        ui->pushButtonRemoveTimeOffset->setEnabled( false );

        on_streamingSpinBox_valueChanged( ui->streamingSpinBox->value() );
    }
    else{
        qDebug() << "Failed to launch the streamer";
    }
}

void MainWindow::onActionLoadLayout(bool reload_previous)
{
    QSettings settings;

    QString directory_path = QDir::currentPath();

    if( settings.contains("MainWindow.lastLayoutDirectory") )
    {
        directory_path = settings.value("MainWindow.lastLayoutDirectory").toString();
    }

    QString filename;
    if( reload_previous && settings.contains("MainWindow.recentlyLoadedLayout") )
    {
        filename = settings.value("MainWindow.recentlyLoadedLayout").toString();
    }
    else{
        filename = QFileDialog::getOpenFileName(this,
                                                "Open Layout",
                                                directory_path,
                                                "*.xml");
    }
    if (filename.isEmpty())
        return;
    else
        onActionLoadLayoutFromFile(filename, true);
}

void MainWindow::loadPluginState(const QDomElement& root)
{
    QDomElement plugins = root.firstChildElement("Plugins");

    if( ! plugins.isNull() )
    {
        for ( QDomElement plugin_elem = plugins.firstChildElement()  ;
              plugin_elem.isNull() == false;
              plugin_elem = plugin_elem.nextSiblingElement() )
        {
            const QString plugin_name = plugin_elem.nodeName();
            if( _data_loader.find(plugin_name) != _data_loader.end() )
            {
                _data_loader[plugin_name]->xmlLoadState(plugin_elem);
            }
            if( _data_streamer.find(plugin_name) != _data_streamer.end() )
            {
                _data_streamer[plugin_name]->xmlLoadState(plugin_elem);
            }
            if( _state_publisher.find(plugin_name) != _state_publisher.end() )
            {
                _state_publisher[plugin_name]->xmlLoadState(plugin_elem);
            }
        }
    }
}

void MainWindow::savePluginState(QDomDocument& doc)
{
    QDomElement root = doc.namedItem("root").toElement();

    QDomElement plugins_elem = doc.createElement( "Plugins" );
    root.appendChild( plugins_elem );

    for (auto& it: _data_loader)
    {
        QString name  = it.first;
        const DataLoader* dataloader = it.second;

        QDomElement elem = doc.createElement( name );
        if( elem.isNull() == false)
        {
            elem.appendChild( dataloader->xmlSaveState(doc) );
        }
        plugins_elem.appendChild( elem );
    }

    for (auto& it: _data_streamer)
    {
        QString name = it.first;
        const DataStreamer* datastreamer = it.second;

        QDomElement elem = doc.createElement(  name );
        elem.appendChild( datastreamer->xmlSaveState(doc) );
        plugins_elem.appendChild( elem );
    }

    for (auto& it: _state_publisher)
    {
        QString name = it.first;
        const StatePublisher* state_publisher = it.second;

        QDomElement elem = doc.createElement(  name );
        elem.appendChild( state_publisher->xmlSaveState(doc) );
        plugins_elem.appendChild( elem );
    }
}

std::tuple<double, double, int> MainWindow::calculateVisibleRangeX()
{
    // find min max time
    double min_time =  std::numeric_limits<double>::max();
    double max_time = -std::numeric_limits<double>::max();
    int max_steps = 0;

    forEachWidget([&](PlotWidget* widget)
    {
        for (auto& it: widget->curveList())
        {
            const auto& curve_name = it.first;

            const auto& data = _mapped_plot_data.numeric.find(curve_name)->second;
            if(data.size() >=1)
            {
                const double t0 = data.front().x;
                const double t1 = data.back().x;
                min_time  = std::min( min_time, t0);
                max_time  = std::max( max_time, t1);
                max_steps = std::max( max_steps, (int)data.size());
            }
        }
    });

    // needed if all the plots are empty
    if( max_steps == 0 || max_time < min_time)
    {
        for (const auto& it: _mapped_plot_data.numeric)
        {
            const PlotData& data = it.second;
            if(data.size() >=1)
            {
                const double t0 = data.front().x;
                const double t1 = data.back().x;
                min_time  = std::min( min_time, t0);
                max_time  = std::max( max_time, t1);
                max_steps = std::max( max_steps, (int)data.size());
            }
        }
    }

    // last opportunity. Everything else failed
    if( max_steps == 0 || max_time < min_time)
    {
        min_time = 0.0;
        max_time = 1.0;
        max_steps = 1;
    }
    return std::tuple<double,double,int>( min_time, max_time, max_steps );
}

void MainWindow::onActionLoadLayoutFromFile(QString filename, bool load_data)
{
    QSettings settings;

    QString directory_path = QFileInfo(filename).absolutePath();
    settings.setValue("MainWindow.lastLayoutDirectory",  directory_path);
    settings.setValue("MainWindow.recentlyLoadedLayout", filename);

    ui->actionLoadRecentLayout->setText("Load layout from: " + filename);

    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Layout"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(filename)
                             .arg(file.errorString()));
        return;
    }

    QString errorStr;
    int errorLine, errorColumn;

    QDomDocument domDocument;

    if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn)) {
        QMessageBox::information(window(), tr("XML Layout"),
                                 tr("Parse error at line %1:\n%2")
                                 .arg(errorLine)
                                 .arg(errorStr));
        return;
    }

    //-------------------------------------------------
    // refresh plugins
    QDomElement root = domDocument.namedItem("root").toElement();
    loadPluginState(root);
    //-------------------------------------------------
    if(load_data)
    {
        QDomElement previously_loaded_datafile =  root.firstChildElement( "previouslyLoadedDatafile" );
        if( previously_loaded_datafile.isNull() == false)
        {
            QString filename;

            //new format
            if( previously_loaded_datafile.hasAttribute("filename"))
            {
                filename =  previously_loaded_datafile.attribute("filename");
            }
            else{  // old format
                filename = previously_loaded_datafile.text();
            }

            QMessageBox msgBox;
            msgBox.setWindowTitle("Load Data?");
            msgBox.setText(tr("Do you want to reload the previous datafile?\n\n %1 \n\n").arg(filename));
            msgBox.addButton(tr("No (Layout only)"), QMessageBox::RejectRole);
            QPushButton* buttonBoth = msgBox.addButton(tr("Yes (Both Layout and Datafile)"), QMessageBox::YesRole);
            msgBox.setDefaultButton(buttonBoth);
            msgBox.exec();
            if( msgBox.clickedButton() == buttonBoth )
            {
                onActionLoadDataFileImpl( filename, true );
            }
        }
    }

    QDomElement previously_loaded_streamer =  root.firstChildElement( "previouslyLoadedStreamer" );
    if( previously_loaded_streamer.isNull() == false)
    {
        QString streamer_name = previously_loaded_streamer.attribute("name");

        QMessageBox msgBox;
        msgBox.setWindowTitle("Start Streaming?");
        msgBox.setText(tr("Do you want to start the previously used streaming plugin?\n\n %1 \n\n").arg(streamer_name));
        msgBox.addButton(tr("No (Layout only)"), QMessageBox::RejectRole);
        QPushButton* buttonBoth = msgBox.addButton(tr("Yes (Both Layout and Streaming)"), QMessageBox::YesRole);
        msgBox.setDefaultButton(buttonBoth);
        msgBox.exec();
        if( msgBox.clickedButton() == buttonBoth )
        {
            if( _data_streamer.count(streamer_name) != 0 )
            {
                onActionLoadStreamer( streamer_name );
            }
            else{
                QMessageBox::warning(this, tr("Error Loading Streamer"),
                                     tr("The streamer named %1 can not be loaded.").arg(streamer_name));
            }
        }
    }

    ///--------------------------------------------------

    xmlLoadState( domDocument );

    _undo_states.clear();
    _undo_states.push_back( domDocument );
}


void MainWindow::onUndoInvoked( )
{
    _disable_undo_logging = true;
    if( _undo_states.size() > 1)
    {
        QDomDocument state_document = _undo_states.back();
        while( _redo_states.size() >= 100 ) _redo_states.pop_front();
        _redo_states.push_back( state_document );
        _undo_states.pop_back();
        state_document = _undo_states.back();

        xmlLoadState( state_document );
    }
    _disable_undo_logging = false;
}


void MainWindow::on_tabbedAreaDestroyed(QObject *object)
{
    this->setFocus();
}

void MainWindow::onFloatingWindowDestroyed(QObject *object)
{
    //    for (size_t i=0; i< SubWindow::instances().size(); i++)
    //    {
    //        if( SubWindow::instances()[i] == object)
    //        {
    //            SubWindow::instances().erase( SubWindow::instances().begin() + i);
    //            break;
    //        }
    //    }
}

void MainWindow::onCreateFloatingWindow(PlotMatrix* first_tab)
{
    createTabbedDialog( QString(), first_tab );
}

void MainWindow::forEachWidget(std::function<void (PlotWidget*, PlotMatrix*, int,int )> operation)
{
    auto func = [&](QTabWidget * tabs)
    {
        for (int t=0; t < tabs->count(); t++)
        {
            PlotMatrix* matrix =  static_cast<PlotMatrix*>(tabs->widget(t));

            for(unsigned row=0; row< matrix->rowsCount(); row++)
            {
                for(unsigned col=0; col< matrix->colsCount(); col++)
                {
                    PlotWidget* plot = matrix->plotAt(row, col);
                    operation(plot, matrix, row, col);
                }
            }
        }
    };

    for(const auto& it: TabbedPlotWidget::instances())
    {
        func( it.second->tabWidget() );
    }
}

void MainWindow::forEachWidget(std::function<void (PlotWidget *)> op)
{
    forEachWidget( [&](PlotWidget*plot, PlotMatrix*, int,int) { op(plot); } );
}

void MainWindow::updateTimeSlider()
{
    auto range = calculateVisibleRangeX();

    ui->timeSlider->setLimits(std::get<0>(range) - _time_offset.get(),
                              std::get<1>(range) - _time_offset.get(),
                              std::get<2>(range));
}

void MainWindow::updateTimeOffset()
{
    auto range = calculateVisibleRangeX();
    double min_time = std::get<0>(range);

    const bool remove_offset = ui->pushButtonRemoveTimeOffset->isChecked();
    if( remove_offset && min_time != std::numeric_limits<double>::max())
    {
        _time_offset.set( min_time );
    }
    else{
        _time_offset.set( 0.0 );
    }
}

void MainWindow::onSwapPlots(PlotWidget *source, PlotWidget *destination)
{
    if( !source || !destination ) return;

    PlotMatrix* src_matrix = NULL;
    PlotMatrix* dst_matrix = NULL;
    QPoint src_pos;
    QPoint dst_pos;

    forEachWidget( [&](PlotWidget* plot, PlotMatrix* matrix, int row,int col)
    {
        if( plot == source ) {
            src_matrix = matrix;
            src_pos.setX( row );
            src_pos.setY( col );
        }
        else if( plot == destination )
        {
            dst_matrix = matrix;
            dst_pos.setX( row );
            dst_pos.setY( col );
        }
    });

    if(src_matrix && dst_matrix)
    {
        src_matrix->gridLayout()->removeWidget( source );
        dst_matrix->gridLayout()->removeWidget( destination );

        src_matrix->gridLayout()->addWidget( destination, src_pos.x(), src_pos.y() );
        dst_matrix->gridLayout()->addWidget( source,      dst_pos.x(), dst_pos.y() );

        src_matrix->updateLayout();
        if( src_matrix != dst_matrix){
            dst_matrix->updateLayout();
        }
        source->changeBackgroundColor( QColor( 250, 250, 250 ) );
        destination->changeBackgroundColor( QColor( 250, 250, 250 ) );
    }
    onUndoableChange();
}

void MainWindow::on_pushButtonStreaming_toggled(bool streaming)
{
    if( !_current_streamer )
    {
        streaming = false;
    }

    ui->pushButtonRemoveTimeOffset->setEnabled( !streaming );

    if( streaming )
    {
        ui->horizontalSpacer->changeSize(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->pushButtonStreaming->setText("Streaming ON");
    }
    else{
        _replot_timer->stop( );
        ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->pushButtonStreaming->setText("Streaming OFF");
    }
    ui->streamingLabel->setHidden( !streaming );
    ui->streamingSpinBox->setHidden( !streaming );
    ui->timeSlider->setHidden( streaming );

    forEachWidget([&](PlotWidget* plot)
    {
        plot->enableTracker( !streaming );
    } );

    emit activateStreamingMode( streaming );

    this->repaint();


    if( _current_streamer && streaming)
    {
        _replot_timer->start();
        updateTimeOffset();
    }
    else{
        updateTimeSlider();
        updateDataAndReplot();
        onUndoableChange();
    }
}

void MainWindow::on_ToggleStreaming()
{
    ui->pushButtonStreaming->setChecked( !ui->pushButtonStreaming->isChecked() );
}

void MainWindow::updateDataAndReplot()
{
    if( _current_streamer )
    {
        std::lock_guard<std::mutex> lock( _current_streamer->mutex() );
        importPlotDataMap( _current_streamer->dataMap(), false );
    }

    forEachWidget( [](PlotWidget* plot)
    {
        plot->updateCurves();
    } );

    //--------------------------------
    // trigger again the execution of this callback if steaming == true
    if( isStreamingActive() )
    {
        auto range = calculateVisibleRangeX();
        double max_time = std::get<1>(range);
        _tracker_time = max_time;

        onTrackerTimeUpdated(_tracker_time, false);
    }
    //--------------------------------
    for(const auto& it: TabbedPlotWidget::instances())
    {
        PlotMatrix* matrix =  it.second->currentTab() ;
        matrix->maximumZoomOut(); // includes replot
    }
}

void MainWindow::on_streamingSpinBox_valueChanged(int value)
{
    for (auto& it : _mapped_plot_data.numeric )
    {
        it.second.setMaximumRangeX( value );
    }

    for (auto& it: _mapped_plot_data.user_defined)
    {
        it.second.setMaximumRangeX( value );
    }

    if( _current_streamer )
    {
        std::lock_guard<std::mutex> lock( _current_streamer->mutex() );

        for (auto& it : _current_streamer->dataMap().numeric )
        {
            it.second.setMaximumRangeX( value );
        }

        for (auto& it: _current_streamer->dataMap().user_defined)
        {
            it.second.setMaximumRangeX( value );
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog* aboutdialog = new AboutDialog(this);
    aboutdialog->show();
}

void MainWindow::on_actionStopStreaming_triggered()
{
    ui->pushButtonStreaming->setChecked(false);
    ui->pushButtonStreaming->setEnabled(false);
    _replot_timer->stop();
    _current_streamer->shutdown();
    _current_streamer = nullptr;

    for(auto& action: ui->menuStreaming->actions()) {
        action->setEnabled(true);
    }
    ui->actionStopStreaming->setEnabled(false);

    if( !_mapped_plot_data.numeric.empty()){
        ui->actionDeleteAllData->setEnabled( true );
        ui->actionDeleteAllData->setToolTip("");
    }

    // reset this.
    for(auto& it: _mapped_plot_data.numeric)
    {
        it.second.setMaximumRangeX( std::numeric_limits<double>::max() );
    }
    for(auto& it: _mapped_plot_data.user_defined)
    {
        it.second.setMaximumRangeX( std::numeric_limits<double>::max() );
    }
}


void MainWindow::on_actionExit_triggered()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, tr("Warning"),
                                  tr("Do you really want quit?\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes );
    if( reply == QMessageBox::Yes ) {
        this->close();
    }
}

void MainWindow::on_actionQuick_Help_triggered()
{
    const QString path =  tr(PJ_DOCUMENTATION_DIR) + "/index.html";
    QFileInfo check_file(path);

    QUrl url_SYS( tr("file:///")  + path, QUrl::TolerantMode);

    if( check_file.exists() && url_SYS.isValid() )
    {
        if(QDesktopServices::openUrl(url_SYS))
        {
            return;
        }
    }
    QMessageBox::warning(this, "Can't find Documentation",
                         QString("Can't open the file:\n\n%1\n\n Is it correctly installed in your system?").arg( url_SYS.path()) );
}


void MainWindow::on_pushButtonRemoveTimeOffset_toggled(bool )
{
    updateTimeOffset();
    updatedDisplayTime();
    if (this->signalsBlocked() == false)  onUndoableChange();
}


void MainWindow::on_pushButtonOptions_toggled(bool checked)
{
    ui->widgetOptions->setVisible( checked );
    ui->line->setVisible( checked );
}

void MainWindow::updatedDisplayTime()
{
    const double relative_time = _tracker_time - _time_offset.get();
    if( ui->pushButtonUseDateTime->isChecked() )
    {
        if( _time_offset.get() > 0 )
        {
            QTime time = QTime::fromMSecsSinceStartOfDay( Round(relative_time*1000.0));
            ui->displayTime->setText( time.toString("HH:mm::ss.zzz") );
        }
        else{
            QDateTime datetime = QDateTime::fromMSecsSinceEpoch( Round(relative_time*1000.0) );
            ui->displayTime->setText( datetime.toString("d/M/yy HH:mm::ss.zzz") );
        }
    }
    else{
        ui->displayTime->setText( QString::number(relative_time, 'f', 3));
    }
}

void MainWindow::on_pushButtonActivateGrid_toggled(bool checked)
{
    forEachWidget( [checked](PlotWidget* plot) {
        plot->activateGrid( checked );
        plot->replot();
    });
}

void MainWindow::on_actionClearBuffer_triggered()
{
    for (auto& it: _mapped_plot_data.numeric )
    {
        it.second.clear();
    }

    for (auto& it: _mapped_plot_data.user_defined )
    {
        it.second.clear();
    }

    forEachWidget( [](PlotWidget* plot) {
        plot->reloadPlotData();
        plot->replot();
    });
}

void MainWindow::on_pushButtonUseDateTime_toggled(bool checked)
{
    updatedDisplayTime();
}

void MainWindow::on_pushButtonTimeTracker_pressed()
{
    if( _tracker_param == CurveTracker::LINE_ONLY)
    {
        _tracker_param = CurveTracker::VALUE;
    }
    else if( _tracker_param == CurveTracker::VALUE)
    {
        _tracker_param = CurveTracker::VALUE_NAME;
    }
    else if( _tracker_param == CurveTracker::VALUE_NAME)
    {
        _tracker_param = CurveTracker::LINE_ONLY;
    }
    ui->pushButtonTimeTracker->setIcon( _tracker_button_icons[ _tracker_param ] );

    forEachWidget( [&](PlotWidget* plot) {
        plot->configureTracker(_tracker_param);
        plot->replot();
    });
}

void MainWindow::on_minimizeView()
{
    static bool first_call = true;
    if( first_call && !_minimized )
    {
        first_call = false;
        QMessageBox::information(0,"Remember!", "Press F10 to switch back to the normal view");
    }

    _minimized = !_minimized;

    ui->leftFrame->setVisible(!_minimized);
    ui->widgetOptions->setVisible( !_minimized && ui->pushButtonOptions->isChecked() );
    ui->widgetTimescale->setVisible(!_minimized);
    ui->menuBar->setVisible(!_minimized);

    for (auto& it: TabbedPlotWidget::instances() )
    {
        it.second->setControlsVisible( !_minimized );
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    _replot_timer->stop();
    if( _current_streamer )
    {
        _current_streamer->shutdown();
        _current_streamer = nullptr;
    }
    QSettings settings;
    settings.setValue("MainWindow.geometry", saveGeometry());
    settings.setValue("MainWindow.activateGrid", ui->pushButtonActivateGrid->isChecked() );
    settings.setValue("MainWindow.streamingBufferValue", ui->streamingSpinBox->value() );
    settings.setValue("MainWindow.dateTimeDisplay",ui->pushButtonUseDateTime->isChecked() );
    settings.setValue("MainWindow.timeTrackerSetting", (int)_tracker_param );

    // clean up all the plugins
    for(auto& it : _data_loader ) { delete it.second; }
    for(auto& it : _state_publisher ) { delete it.second; }
    for(auto& it : _data_streamer ) { delete it.second; }
}
