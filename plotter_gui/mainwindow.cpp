#include <functional>
#include <QMouseEvent>
#include <QDebug>
#include <numeric>
#include <QMimeData>
#include <QMenu>
#include <QStringListModel>
#include <stdio.h>
#include <qwt_plot_canvas.h>
#include <QDomDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringRef>
#include <QThread>
#include <QPluginLoader>
#include <QSettings>
#include <QWindow>
#include <set>
#include <QInputDialog>
#include <QCommandLineParser>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "busydialog.h"
#include "busytaskdialog.h"
#include "filterablelistwidget.h"
#include "tabbedplotwidget.h"
#include "selectlistdialog.h"
#include "aboutdialog.h"
#include <QMovie>
#include <QScrollBar>
#include "ui_help_dialog.h"

MainWindow::MainWindow(const QCommandLineParser &commandline_parser, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _undo_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this),
    _redo_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this),
    _current_streamer(nullptr),
    _disable_undo_logging(false)
{
    QLocale::setDefault(QLocale::c()); // set as default

    _test_option = (commandline_parser.isSet("test"));

    _curvelist_widget = new FilterableListWidget(this);
    _streamer_signal_mapper = new QSignalMapper(this);

    ui->setupUi(this);

    connect( _curvelist_widget->getTtable()->verticalScrollBar(), SIGNAL(sliderMoved(int)),
             this, SLOT(updateLeftTableValues()) );

    connect( _curvelist_widget, SIGNAL(hiddenItemsChanged()),
             this, SLOT(updateLeftTableValues()) );

    connect(_curvelist_widget, SIGNAL(deleteCurve(QString)),
            this, SLOT(deleteLoadedData(QString)) );

    connect(this, SIGNAL(trackerTimeUpdated(QPointF)), this, SLOT(updateLeftTableValues()) );

    _main_tabbed_widget = new TabbedPlotWidget( this,  &_mapped_plot_data, this);

    ui->centralLayout->insertWidget(0, _main_tabbed_widget);
    ui->leftLayout->addWidget( _curvelist_widget );

    ui->splitter->setCollapsible(0,true);
    ui->splitter->setStretchFactor(0,2);
    ui->splitter->setStretchFactor(1,5);

    connect( ui->splitter, SIGNAL(splitterMoved(int,int)), SLOT(onSplitterMoved(int,int)) );

    createActions();

    loadPlugins( QCoreApplication::applicationDirPath() );
    loadPlugins("/usr/local/PlotJuggler/plugins");

    _undo_timer.start();

    // save initial state
    onUndoableChange();

    _replot_timer = new QTimer(this);
    connect(_replot_timer, SIGNAL(timeout()), this, SLOT(onReplotRequested()));

    ui->menuFile->setToolTipsVisible(true);
    ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->streamingLabel->setHidden(true);
    ui->streamingSpinBox->setHidden(true);

    this->setMenuBar(ui->menuBar);
    ui->menuBar->setNativeMenuBar(false);

    connect(_streamer_signal_mapper, SIGNAL(mapped(QString)),
            this, SLOT(onActionLoadStreamer(QString)) );

    if( _test_option )
    {
        buildData();
    }

    bool file_loaded = false;
    if( commandline_parser.isSet("datafile"))
    {
        QMetaObject::invokeMethod(this, "onActionLoadDataFileImpl",
                                  Q_ARG(QString, commandline_parser.value("datafile")) );
        file_loaded = true;
    }
    if( commandline_parser.isSet("layout"))
    {
        QMetaObject::invokeMethod(this, "onActionLoadLayoutFromFile",
                                  Q_ARG(QString, commandline_parser.value("layout")),
                                  Q_ARG(bool, file_loaded ) );
    }

    QSettings settings( "IcarusTechnology", "PlotJuggler");
    restoreGeometry(settings.value("MainWindow.geometry").toByteArray());
}

MainWindow::~MainWindow()
{
    if( _current_streamer )
    {
        _current_streamer->shutdown();
        _current_streamer = nullptr;
    }
    QSettings settings( "IcarusTechnology", "PlotJuggler");
    settings.setValue("MainWindow.geometry", saveGeometry());
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

    if( ui->pushButtonStreaming->isChecked() == false)
    {
        while( _undo_states.size() >= 100 ) _undo_states.pop_front();
        _undo_states.push_back( xmlSaveState() );
        updateInternalState();
        _redo_states.clear();
        //qDebug() << "Undo pushed " <<  _undo_states.size();
    }
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

        //qDebug() << "ReDo pushed " <<  _undo_states.size();

        updateInternalState();
    }
    _disable_undo_logging = false;
}


void MainWindow::updateLeftTableValues()
{
    const auto& table = _curvelist_widget->getTtable();

    if( table->isColumnHidden(1) == false)
    {
        const int vertical_height = table->visibleRegion().boundingRect().height();

        for (int row = 0; row < _curvelist_widget->rowCount(); row++)
        {
            int vertical_pos = table->rowViewportPosition(row);
            if( vertical_pos < 0 || table->isRowHidden(row) ){   continue; }
            if( vertical_pos > vertical_height){ break; }

            const std::string name = table->item(row,0)->text().toStdString();
            auto it = _mapped_plot_data.numeric.find(name);
            if( it !=  _mapped_plot_data.numeric.end())
            {
                nonstd::optional<PlotData::TimeType> value;
                PlotDataPtr data = it->second;

                double num = 0.0;
                bool valid = false;

                if( _tracker_time < std::numeric_limits<double>::max())
                {
                    auto value = data->getYfromX( _tracker_time );
                    if(value){
                        valid = true;
                        num = value.value();
                    }
                }
                else{
                    if( data->size() > 0) {
                        valid = true;
                        num = (data->at( data->size()-1 )).y;
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
                    table->item(row,1)->setText(num_text + ' ');
                }
            }
        }
    }
}

void MainWindow::onTrackerTimeUpdated(double current_time)
{
    if( current_time < _min_slider_time ) current_time = _min_slider_time;
    if( current_time > _max_slider_time ) current_time = _max_slider_time;

    double ratio = (current_time - _min_slider_time)/(double)(_max_slider_time-_min_slider_time);
    double min_slider = (double)ui->horizontalSlider->minimum();
    double max_slider = (double)ui->horizontalSlider->maximum();
    int slider_value = (int)((max_slider- min_slider)* ratio) ;

    ui->horizontalSlider->setValue(slider_value);

    //------------------------
    for ( auto it: _state_publisher)
    {
        it.second->updateState( &_mapped_plot_data, current_time);
    }
    ui->displayTime->setText( QString::number(current_time, 'f', 3));
}

void MainWindow::onTrackerPositionUpdated(QPointF pos)
{
    onTrackerTimeUpdated( pos.x() );
    _tracker_time = pos.x();
    emit  trackerTimeUpdated( QPointF(pos ) );
}

void MainWindow::createTabbedDialog(PlotMatrix* first_tab)
{
    SubWindow* window = new SubWindow(&_mapped_plot_data, this );
    Qt::WindowFlags flags = window->windowFlags();
    window->setWindowFlags( flags | Qt::SubWindow );

    const char prefix[] = "Window ";
    int window_number = 1;

    bool number_taken = true;
    while( number_taken )
    {
        number_taken = false;
        for (const auto& floating_window: _floating_window )
        {
            QString win_title = floating_window->windowTitle();
            win_title.remove(0, sizeof(prefix)-1 );
            int num = win_title.toInt();

            if (num == window_number)
            {
                number_taken = true;
                window_number++;
                break;
            }
        }
    }

    window->setWindowTitle( QString(prefix) + QString::number(window_number));

    connect( window, SIGNAL(destroyed(QObject*)),    this,  SLOT(onFloatingWindowDestroyed(QObject*)) );
    connect( window, SIGNAL(closeRequestedByUser()), this,  SLOT(onUndoableChange()) );

    _floating_window.push_back( window );

    // TabbedPlotWidget *tabbed_widget = new TabbedPlotWidget( &_mapped_plot_data, window);
    //_tabbed_plotarea.push_back( tabbed_widget );

    window->tabbedWidget()->setStreamingMode( ui->pushButtonStreaming->isChecked() );

    window->setAttribute( Qt::WA_DeleteOnClose, true );
    window->show();
    window->activateWindow();
    window->raise();

    if (this->signalsBlocked() == false) onUndoableChange();
}


void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    QStringList mimeFormats = mimeData->formats();

    foreach(QString format, mimeFormats)
    {
        // qDebug() << " mimestuff " << format;
    }
}

void MainWindow::createActions()
{
    _undo_shortcut.setContext(Qt::ApplicationShortcut);
    _redo_shortcut.setContext(Qt::ApplicationShortcut);

    connect( &_undo_shortcut, SIGNAL(activated()), this, SLOT(onUndoInvoked()) );
    connect( &_redo_shortcut, SIGNAL(activated()), this, SLOT(onRedoInvoked()) );

    //---------------------------------------------

    connect( ui->actionSaveLayout,SIGNAL(triggered()),        this, SLOT(onActionSaveLayout()) );
    connect(ui->actionLoadLayout,SIGNAL(triggered()),         this, SLOT(onActionLoadLayout()) );
    connect(ui->actionLoadData,SIGNAL(triggered()),           this, SLOT(onActionLoadDataFile()) );
    connect(ui->actionLoadRecentDatafile,SIGNAL(triggered()), this, SLOT(onActionReloadDataFileFromSettings()) );
    connect(ui->actionLoadRecentLayout,SIGNAL(triggered()),   this, SLOT(onActionReloadRecentLayout()) );
    connect(ui->actionReloadData,SIGNAL(triggered()),         this, SLOT(onActionReloadSameDataFile()) );
    connect(ui->actionDeleteAllData,SIGNAL(triggered()),      this, SLOT(onDeleteLoadedData()) );

    //---------------------------------------------

    QSettings settings( "IcarusTechnology", "PlotJuggler");
    if( settings.contains("MainWindow.recentlyLoadedDatafile") )
    {
        QString filename = settings.value("MainWindow.recentlyLoadedDatafile").toString();
        ui->actionLoadRecentDatafile->setText( "Load data from: " + filename);
        ui->actionLoadRecentDatafile->setEnabled( true );
    }
    else{
        ui->actionLoadRecentDatafile->setEnabled( false );
    }

    ui->actionReloadData->setEnabled( false );
    ui->actionDeleteAllData->setEnabled( false );

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

    foreach (QString filename, pluginsDir.entryList(QDir::Files))
    {
        if( loaded_plugins.find( filename ) != loaded_plugins.end())
        {
            continue;
        }

        QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(filename));

        QObject *plugin = pluginLoader.instance();
        if (plugin)
        {
            DataLoader *loader        = qobject_cast<DataLoader *>(plugin);
            StatePublisher *publisher = qobject_cast<StatePublisher *>(plugin);
            DataStreamer *streamer    =  qobject_cast<DataStreamer *>(plugin);

            if (loader)
            {
                qDebug() << filename << ": is a DataLoader plugin";
                if( !_test_option && loader->isDebugPlugin())
                {
                    qDebug() << filename << "...but will be ignored unless the argument -t is used.";
                }
                else{
                    loaded_plugins.insert( loader->name() );
                    _data_loader.insert( std::make_pair( loader->name(), loader) );
                }
            }
            else if (publisher)
            {
                qDebug() << filename << ": is a StatePublisher plugin";
                if( !_test_option && publisher->isDebugPlugin())
                {
                    qDebug() << filename << "...but will be ignored unless the argument -t is used.";
                }
                else
                {
                    loaded_plugins.insert( publisher->name() );
                    _state_publisher.insert( std::make_pair(publisher->name(), publisher) );

                    QAction* activatePublisher = new QAction( publisher->name() , this);
                    activatePublisher->setCheckable(true);
                    activatePublisher->setChecked(false);
                    ui->menuPublishers->setEnabled(true);
                    ui->menuPublishers->addAction(activatePublisher);

                    connect(activatePublisher, SIGNAL( toggled(bool)),
                            publisher->getObject(), SLOT(setEnabled(bool)) );
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
                    QString name(streamer->name());
                    loaded_plugins.insert( name );
                    _data_streamer.insert( std::make_pair(name , streamer ) );

                    QAction* startStreamer = new QAction(QString("Start: ") + name, this);
                    ui->menuStreaming->setEnabled(true);
                    ui->menuStreaming->addAction(startStreamer);

                    streamer->setMenu( ui->menuStreaming );

                    connect(startStreamer, SIGNAL(triggered()), _streamer_signal_mapper, SLOT(map()));
                    _streamer_signal_mapper->setMapping(startStreamer, name );
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

void MainWindow::buildData()
{
    size_t SIZE = 100*1000;

    QStringList  words_list;
    words_list << "siam" << "tre" << "piccoli" << "porcellin"
               << "mai" << "nessun" << "ci" << "dividera";

    for(auto& word: words_list){
        _curvelist_widget->addItem( new QTableWidgetItem(word) );
    }

    foreach( const QString& name, words_list)
    {
        double A =  6* ((double)qrand()/(double)RAND_MAX)  - 3;
        double B =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double C =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double D =  20* ((double)qrand()/(double)RAND_MAX)  ;

        PlotDataPtr plot ( new PlotData( name.toStdString().c_str() ) );

        double t = 0;
        for (unsigned indx=0; indx<SIZE; indx++)
        {
            t += 0.001;
            plot->pushBack( PlotData::Point( t,  A*sin(B*t + C) + D*t*0.02 ) ) ;
        }
        _mapped_plot_data.numeric.insert( std::make_pair( name.toStdString(), plot) );
    }

    //---------------------------------------
    PlotDataPtr sin_plot ( new PlotData( "_sin" ) );
    PlotDataPtr cos_plot ( new PlotData( "_cos" ) );

    double t = 0;
    for (unsigned indx=0; indx<SIZE; indx++)
    {
        t += 0.001;
        sin_plot->pushBack( PlotData::Point( t,  1.0*sin(t*0.4) ) ) ;
        cos_plot->pushBack( PlotData::Point( t,  2.0*cos(t*0.4) ) ) ;
    }

    _mapped_plot_data.numeric.insert( std::make_pair( sin_plot->name(), sin_plot) );
    _mapped_plot_data.numeric.insert( std::make_pair( cos_plot->name(), cos_plot) );

    _curvelist_widget->addItem( new QTableWidgetItem(sin_plot->name().c_str()) );
    _curvelist_widget->addItem( new QTableWidgetItem(cos_plot->name().c_str()) );
    //--------------------------------------

    updateTimeSlider();

    _curvelist_widget->updateFilter();

    forEachWidget( [](PlotWidget* plot) {
        plot->reloadPlotData();
    } );
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
    connect( plot, SIGNAL(undoableChange()),       this, SLOT(onUndoableChange()) );
    connect( plot, SIGNAL(trackerMoved(QPointF)),  this, SLOT(onTrackerPositionUpdated(QPointF)));
    connect( plot, SIGNAL(swapWidgetsRequested(PlotWidget*,PlotWidget*)), this, SLOT(onSwapPlots(PlotWidget*,PlotWidget*)) );

    connect( this, SIGNAL(requestRemoveCurveByName(const QString&)), plot, SLOT(removeCurve(const QString&))) ;

    connect( this, SIGNAL(trackerTimeUpdated(QPointF)), plot, SLOT(setTrackerPosition(QPointF)));
    connect( this, SIGNAL(trackerTimeUpdated(QPointF)), plot, SLOT( replot() ));

    connect( this, SIGNAL(activateTracker(bool)),  plot, SLOT(activateTracker(bool)) );
    connect( this, SIGNAL(activateTracker(bool)),  plot, SLOT( replot() ));

    plot->activateTracker( ui->pushButtonActivateTracker->isChecked() );
}

void MainWindow::onPlotMatrixAdded(PlotMatrix* matrix)
{
    connect( matrix, SIGNAL(plotAdded(PlotWidget*)), this, SLOT( onPlotAdded(PlotWidget*)));
    connect( matrix, SIGNAL(undoableChange()),       this, SLOT( onUndoableChange()) );
}

QDomDocument MainWindow::xmlSaveState() const
{
    QDomDocument doc;
    QDomProcessingInstruction instr =
            doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");

    doc.appendChild(instr);

    QDomElement root = doc.createElement( "root" );

    QDomElement main_area =_main_tabbed_widget->xmlSaveState(doc);
    root.appendChild( main_area );

    for (SubWindow* floating_window: _floating_window)
    {
        QDomElement tabbed_area = floating_window->tabbedWidget()->xmlSaveState(doc);
        root.appendChild( tabbed_area );
    }

    doc.appendChild(root);

    return doc;
}

bool MainWindow::xmlLoadState(QDomDocument state_document)
{
    const bool isBlocked = this->blockSignals(true);

    QDomElement root = state_document.namedItem("root").toElement();
    if ( root.isNull() ) {
        qWarning() << "No <root> element found at the top-level of the XML file!";
        return false;
    }

    QDomElement tabbed_area;

    size_t num_floating = 0;

    for (  tabbed_area = root.firstChildElement(  "tabbed_widget" )  ;
           tabbed_area.isNull() == false;
           tabbed_area = tabbed_area.nextSiblingElement( "tabbed_widget" ) )
    {
        if( tabbed_area.attribute("parent").compare("main_window") != 0)
        {
            num_floating++;
        }
    }

    // add windows if needed
    while( _floating_window.size() < num_floating )
    {
        createTabbedDialog( NULL );
    }

    while( _floating_window.size() > num_floating ){
        QMainWindow* window =  _floating_window.back();
        _floating_window.pop_back();
        window->deleteLater();
    }

    //-----------------------------------------------------
    size_t index = 0;

    for (  tabbed_area = root.firstChildElement(  "tabbed_widget" )  ;
           tabbed_area.isNull() == false;
           tabbed_area = tabbed_area.nextSiblingElement( "tabbed_widget" ) )
    {
        if( tabbed_area.attribute("parent").compare("main_window") == 0)
        {
            _main_tabbed_widget->xmlLoadState( tabbed_area );
        }
        else{
            _floating_window[index++]->tabbedWidget()->xmlLoadState( tabbed_area );
        }
    }
    this->blockSignals(isBlocked);
    return true;
}

void MainWindow::onActionSaveLayout()
{
    QDomDocument doc = xmlSaveState();

    if( _loaded_datafile.isEmpty() == false)
    {
        QDomElement root = doc.namedItem("root").toElement();
        QDomElement previously_loaded_datafile =  doc.createElement( "previouslyLoadedDatafile" );
        QDomText textNode = doc.createTextNode( _loaded_datafile );
        previously_loaded_datafile.appendChild( textNode );
        root.appendChild( previously_loaded_datafile );
    }
    if( _current_streamer )
    {
        QDomElement root = doc.namedItem("root").toElement();
        QDomElement loaded_streamer =  doc.createElement( "previouslyLoadedStreamer" );
        QDomText textNode = doc.createTextNode( _current_streamer->name() );
        loaded_streamer.appendChild( textNode );
        root.appendChild( loaded_streamer );
    }

    QSettings settings( "IcarusTechnology", "PlotJuggler");

    QString directory_path  = settings.value("MainWindow.lastLayoutDirectory",
                                             QDir::currentPath() ). toString();

    QString filename = QFileDialog::getSaveFileName(this, "Save Layout", directory_path, "*.xml");
    if (filename.isEmpty())
        return;

    if(filename.endsWith(".xml",Qt::CaseInsensitive) == false)
    {
        filename.append(".xml");
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << doc.toString() << endl;
    }
}

void MainWindow::deleteLoadedData(const QString& curve_name)
{
    auto plot_curve = _mapped_plot_data.numeric.find( curve_name.toStdString() );
    if( plot_curve == _mapped_plot_data.numeric.end())
    {
        return;
    }

    _mapped_plot_data.numeric.erase( plot_curve );

    auto rows_to_remove = _curvelist_widget->findRowsByName( curve_name );
    for(int row : rows_to_remove)
    {
        _curvelist_widget->removeRow(row);
    }

    emit requestRemoveCurveByName( curve_name );


    if( _curvelist_widget->rowCount() == 0)
    {
        ui->actionReloadData->setEnabled( false );
        ui->actionDeleteAllData->setEnabled( false );
    }
}


void MainWindow::onDeleteLoadedData()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, tr("Warning"),
                                  tr("Do you really want to remove the loaded data?\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No );
    if( reply == QMessageBox::No ) {
        return;
    }

    _mapped_plot_data.numeric.clear();
    _mapped_plot_data.user_defined.clear();

    _curvelist_widget->clear();

    forEachWidget( [](PlotWidget* plot) { plot->detachAllCurves(); } );

    ui->actionReloadData->setEnabled( false );
    ui->actionDeleteAllData->setEnabled( false );

}

void MainWindow::onActionLoadDataFile(bool reload_from_settings)
{
    if( _data_loader.empty())
    {
        QMessageBox::warning(0, tr("Warning"),
                             tr("No plugin was loaded to process a data file\n") );
        return;
    }

    QSettings settings( "IcarusTechnology", "PlotJuggler");

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

    for (auto it = extensions.begin(); it != extensions.end(); it++)
    {
        file_extension_filter.append( QString(" *.") + *it );
    }

    QString directory_path = settings.value("MainWindow.lastDatafileDirectory", QDir::currentPath() ).toString();

    QString filename;
    if( reload_from_settings && settings.contains("MainWindow.recentlyLoadedDatafile") )
    {
        filename = settings.value("MainWindow.recentlyLoadedDatafile").toString();
    }
    else{
        filename = QFileDialog::getOpenFileName(this,
                                                "Open Datafile",
                                                directory_path,
                                                file_extension_filter);
    }

    if (filename.isEmpty()) {
        return;
    }

    directory_path = QFileInfo(filename).absolutePath();

    settings.setValue("MainWindow.lastDatafileDirectory", directory_path);
    settings.setValue("MainWindow.recentlyLoadedDatafile", filename);

    ui->actionLoadRecentDatafile->setText("Load data from: " + filename);

    onActionLoadDataFileImpl(filename, false );
}

void MainWindow::importPlotDataMap(const PlotDataMap& new_data)
{
    // overwrite the old user_defined map
    _mapped_plot_data.user_defined = new_data.user_defined;

    for (auto& it: new_data.numeric)
    {
        const std::string& name  = it.first;
        PlotDataPtr plot  = it.second;

        QString qname = QString::fromStdString(name);
        auto plot_with_same_name = _mapped_plot_data.numeric.find(name);

        // this is a new plot
        if( plot_with_same_name == _mapped_plot_data.numeric.end() )
        {
            _curvelist_widget->addItem( new QTableWidgetItem( qname ) );
            _mapped_plot_data.numeric.insert( std::make_pair(name, plot) );
        }
        else{ // a plot with the same name existed already, overwrite it
            plot_with_same_name->second = plot;
        }
    }

    if( _mapped_plot_data.numeric.size() > new_data.numeric.size() )
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(0, tr("Warning"),
                                      tr("Do you want to remove the previously loaded data?\n"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::Yes );
        if( reply == QMessageBox::Yes )
        {
            bool repeat = true;
            while( repeat )
            {
                repeat = false;
                for (auto& it: _mapped_plot_data.numeric )
                {
                    auto& name = it.first;
                    if( new_data.numeric.find( name ) == new_data.numeric.end() )
                    {
                        this->deleteLoadedData( QString( name.c_str() ) );
                        repeat = true;
                        break;
                    }
                }
            }
        }
    }

    forEachWidget( [](PlotWidget* plot) {
        plot->reloadPlotData();
    } );

    onReplotRequested();

    updateInternalState();
}

void MainWindow::onActionLoadDataFileImpl(QString filename, bool reuse_last_timeindex )
{
    const QString extension = QFileInfo(filename).suffix().toLower();

    DataLoader* loader = nullptr;

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

    if( compatible_loaders.size() == 1)
    {
        loader = compatible_loaders.front()->second;
    }
    else{
        static QString last_plugin_name_used;

        QStringList names;
        for (auto cl: compatible_loaders)
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
        QString plugin_name = QInputDialog::getItem(this, tr("QInputDialog::getItem()"), tr("Select the loader to use:"), names, 0, false, &ok);
        if (ok && !plugin_name.isEmpty())
        {
            loader = _data_loader[ plugin_name ];
            last_plugin_name_used = plugin_name;
        }
    }

    if( loader )
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
        ui->actionReloadData->setEnabled( true );
        ui->actionDeleteAllData->setEnabled( true );

        std::string timeindex_name_empty;
        std::string & timeindex_name = timeindex_name_empty;
        if( reuse_last_timeindex )
        {
            timeindex_name = _last_load_configuration;
        }

        PlotDataMap mapped_data = loader->readDataFromFile(
                    filename.toStdString(),
                    timeindex_name   );

        _last_load_configuration = timeindex_name;

        // remap to different type
        importPlotDataMap(mapped_data);
    }
    else{
        QMessageBox::warning(this, tr("Error"),
                             tr("Cannot read files with extension %1.\n No plugin can handle that!\n")
                             .arg(filename) );
    }
    _curvelist_widget->updateFilter();
}

void MainWindow::onActionReloadSameDataFile()
{
    onActionLoadDataFileImpl(_loaded_datafile, true );
}

void MainWindow::onActionReloadDataFileFromSettings()
{
    onActionLoadDataFile( true );
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
            qDebug() << "Error. The streame " << streamer_name <<
                        " can't be loaded";
            return;
        }
    }

    if( _current_streamer && _current_streamer->start() )
    {
        _current_streamer->enableStreaming( false );
        ui->pushButtonStreaming->setEnabled(true);
        importPlotDataMap( _current_streamer->getDataMap() );

        for(auto& action: ui->menuStreaming->actions()) {
            action->setEnabled(false);
        }
        ui->actionStopStreaming->setEnabled(true);
        ui->actionDeleteAllData->setEnabled( false );
        ui->actionDeleteAllData->setToolTip("Stop streaming to be able to delete the data");

        ui->pushButtonStreaming->setChecked(true);

        on_streamingSpinBox_valueChanged( ui->streamingSpinBox->value() );
    }
    else{
        qDebug() << "Failed to launch the streamer";
    }
}

void MainWindow::onActionLoadLayout(bool reload_previous)
{
    QSettings settings( "IcarusTechnology", "PlotJuggler");

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

void MainWindow::onActionLoadLayoutFromFile(QString filename, bool load_data)
{
    QSettings settings( "IcarusTechnology", "PlotJuggler");

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

    QDomElement root = domDocument.namedItem("root").toElement();

    if(load_data)
    {
        QDomElement previously_loaded_datafile =  root.firstChildElement( "previouslyLoadedDatafile" );
        if( previously_loaded_datafile.isNull() == false)
        {
            QString filename = previously_loaded_datafile.text();

            QMessageBox::StandardButton reload_previous;
            reload_previous = QMessageBox::question(0, tr("Wait!"),
                                                    tr("Do you want to reload the datafile?\n\n[%1]\n").arg(filename),
                                                    QMessageBox::Yes | QMessageBox::No,
                                                    QMessageBox::Yes );

            if( reload_previous == QMessageBox::Yes ) {
                onActionLoadDataFileImpl( filename );
            }
        }

        QDomElement previously_loaded_streamer =  root.firstChildElement( "previouslyLoadedStreamer" );
        if( previously_loaded_streamer.isNull() == false)
        {
            QString streamer_name = previously_loaded_streamer.text();

            bool streamer_loaded = false;
            for(auto& it: _data_streamer) {
                if( it.first == streamer_name) streamer_loaded = true;
            }
            if( streamer_loaded ){
                onActionLoadStreamer( streamer_name );
            }
            else{
                QMessageBox::warning(this, tr("Error Loading Streamer"),
                                     tr("The stramer named %1 can not be loaded.").arg(streamer_name));
            }
        }
    }

    ///--------------------------------------------------

    xmlLoadState( domDocument );

    _undo_states.clear();
    _undo_states.push_back( domDocument );

    updateInternalState();
}


void MainWindow::onUndoInvoked( )
{
    // qDebug() << "on_UndoInvoked "<<_undo_states.size() << " -> " <<_undo_states.size()-1;

    _disable_undo_logging = true;
    if( _undo_states.size() > 1)
    {
        QDomDocument state_document = _undo_states.back();
        while( _redo_states.size() >= 100 ) _redo_states.pop_front();
        _redo_states.push_back( state_document );
        _undo_states.pop_back();
        state_document = _undo_states.back();

        xmlLoadState( state_document );

        updateInternalState();
    }
    _disable_undo_logging = false;
}


void MainWindow::on_tabbedAreaDestroyed(QObject *object)
{
    updateInternalState();
    this->setFocus();
}

void MainWindow::onFloatingWindowDestroyed(QObject *object)
{
    for (size_t i=0; i< _floating_window.size(); i++)
    {
        if( _floating_window[i] == object)
        {
            _floating_window.erase( _floating_window.begin() + i);
            break;
        }
    }
    updateInternalState();
}

void MainWindow::onCreateFloatingWindow(PlotMatrix* first_tab)
{
    createTabbedDialog( first_tab );
}


void MainWindow::updateInternalState()
{
    std::map<QString,TabbedPlotWidget*> tabbed_map;
    tabbed_map.insert( std::make_pair( QString("Main window"), _main_tabbed_widget) );

    updateTimeSlider();

    for (SubWindow* subwin: _floating_window)
    {
        tabbed_map.insert( std::make_pair( subwin->windowTitle(), subwin->tabbedWidget() ) );
    }
    for (auto& it: tabbed_map)
    {
        it.second->setSiblingsList( tabbed_map );
    }

    if( !ui->pushButtonStreaming->isChecked())
        emit activateTracker(  ui->pushButtonActivateTracker->isChecked() );
    else
        emit activateTracker( false );
}

void MainWindow::forEachWidget(std::function<void (PlotWidget*, PlotMatrix*, int,int )> operation)
{
    std::vector<TabbedPlotWidget*> tabbed_plotarea;
    tabbed_plotarea.reserve( 1+ _floating_window.size());

    tabbed_plotarea.push_back( _main_tabbed_widget );
    for (SubWindow* subwin: _floating_window){
        tabbed_plotarea.push_back( subwin->tabbedWidget() );
    }

    for(size_t w=0; w < tabbed_plotarea.size(); w++)
    {
        QTabWidget * tabs = tabbed_plotarea[w]->tabWidget();

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
    }
}

void MainWindow::forEachWidget(std::function<void (PlotWidget *)> op)
{
    forEachWidget( [&](PlotWidget*plot, PlotMatrix*, int,int) { op(plot); } );
}

void MainWindow::updateTimeSlider()
{
    int max_steps = 10;
    _min_slider_time = std::numeric_limits<double>::max();
    _max_slider_time = std::numeric_limits<double>::min();

    for (auto it: _mapped_plot_data.numeric )
    {
        PlotDataPtr data = it.second;
        if(data->size() >=1)
        {
            const double min = data->at(0).x;
            const double max = data->at( data->size() -1).x;
            if( _min_slider_time > min) _min_slider_time = min;
            if( _max_slider_time < max) _max_slider_time = max;
            if( max_steps < data->size()) max_steps = data->size();
        }
    }
    if( _max_slider_time <= _min_slider_time)
    {
       _min_slider_time = 0.0;
       _max_slider_time = 1.0;
    }

    ui->horizontalSlider->setRange(0,max_steps);

}

void MainWindow::on_pushButtonAddSubwindow_pressed()
{
    createTabbedDialog( NULL );
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
    }
    onUndoableChange();
}

void MainWindow::on_pushButtonStreaming_toggled(bool checked)
{
    if( !_current_streamer )
    {
        checked = false;
    }

    if( checked )
    {
        ui->horizontalSpacer->changeSize(1,1, QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->pushButtonStreaming->setText("Streaming ON");
    }
    else{
        ui->horizontalSpacer->changeSize(0,0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->pushButtonStreaming->setText("Streaming OFF");
    }
    ui->streamingLabel->setHidden( !checked );
    ui->streamingSpinBox->setHidden( !checked );
    ui->horizontalSlider->setHidden( checked );

    emit activateStreamingMode( checked );

    this->repaint();

    if( _current_streamer )
    {
        _current_streamer->enableStreaming( checked ) ;
        _replot_timer->setSingleShot(true);
        _replot_timer->start( 5 );
    }

    if( !checked )
        emit activateTracker(  ui->pushButtonActivateTracker->isChecked() );
    else
        emit activateTracker( false );
}

void MainWindow::onReplotRequested()
{
    bool updated = false;

    _tracker_time = std::numeric_limits<double>::max();

    {
        PlotData::asyncPushMutex().lock();

        for(auto it : _mapped_plot_data.numeric)
        {
            PlotDataPtr data = ( it.second );
            updated |= data->flushAsyncBuffer();
        }

        PlotData::asyncPushMutex().unlock();
    }

    if( updated )
    {
        forEachWidget( [](PlotWidget* plot)
        {
                plot->updateCurves(true);
        } );
    }

    _main_tabbed_widget->currentTab()->maximumZoomOut() ;

    for(SubWindow* subwin: _floating_window)
    {
        PlotMatrix* matrix =  subwin->tabbedWidget()->currentTab() ;
        matrix->maximumZoomOut(); // includes replot
    }

    if( ui->pushButtonStreaming->isChecked())
    {
        _replot_timer->setSingleShot(true);
        _replot_timer->stop( );
        _replot_timer->start( 20 ); // 50 Hz at most

        for (auto it: _mapped_plot_data.numeric)
        {
            it.second->flushAsyncBuffer();
        }

        updateLeftTableValues();
        updateTimeSlider();

        forEachWidget( [&](PlotWidget* plot)
        {
            if( plot->isXYPlot()){
                plot->setTrackerPosition( QPointF( _max_slider_time, 0.0));
            }
        } );
    }
}

void MainWindow::on_streamingSpinBox_valueChanged(int value)
{
    for (auto it : _mapped_plot_data.numeric )
    {
        PlotDataPtr plot = it.second;
        plot->setMaximumRangeX( value );
    }

    for (auto it: _mapped_plot_data.user_defined)
    {
        PlotDataAnyPtr plot = it.second;
        plot->setMaximumRangeX( value );
    }
}

void MainWindow::on_pushButtonActivateTracker_toggled(bool checked)
{
    emit  activateTracker( checked );
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
    QDialog*  dialog = new QDialog(this);
    Ui::HelpDialog *ui = new Ui::HelpDialog;
    ui->setupUi(dialog);

    QMovie *movie_1 = new QMovie(":/doc/pj-drag-curve.gif");
    ui->label_1->setMovie(movie_1);
    movie_1->start();

    QMovie *movie_2 = new QMovie(":/doc/pj-multiplot.gif");
    ui->label_2->setMovie(movie_2);
    movie_2->start();

    QMovie *movie_3 = new QMovie(":/doc/pj-area-zoom.gif");
    ui->label_3->setMovie(movie_3);
    movie_3->start();

    QMovie *movie_4 = new QMovie(":/doc/pj-wheel-zoom.gif");
    ui->label_4->setMovie(movie_4);
    movie_4->start();

    QMovie *movie_5 = new QMovie(":/doc/pj-tracker.gif");
    ui->label_5->setMovie(movie_5);
    movie_5->start();

    QMovie *movie_6 = new QMovie(":/doc/pj-swap-plots.gif");
    ui->label_6->setMovie(movie_6);
    movie_6->start();

    dialog->exec();
}

void MainWindow::on_horizontalSlider_valueChanged(int position)
{
    //qDebug() <<position;
    QSlider* slider = ui->horizontalSlider;
    double ratio = (double)position / (double)(slider->maximum() -  slider->minimum() );
    double posX = (_max_slider_time-_min_slider_time) * ratio + _min_slider_time;

    onTrackerTimeUpdated( posX );
    _tracker_time = posX;
    emit trackerTimeUpdated( QPointF(posX,0 ) );
}
