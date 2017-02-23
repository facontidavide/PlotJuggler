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

    _main_tabbed_widget = new TabbedPlotWidget( this,  &_mapped_plot_data, this);

    ui->centralLayout->insertWidget(0, _main_tabbed_widget);
    ui->leftLayout->addWidget( _curvelist_widget );

    ui->splitter->setCollapsible(0,true);

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

void MainWindow::getMaximumRangeX(double* minX, double* maxX)
{
    *minX = std::numeric_limits<double>::max();
    *maxX = std::numeric_limits<double>::min();

    forEachWidget( [&](PlotWidget* plot)
    {
        auto rangeX = plot->maximumRangeX();
        if( *minX > rangeX.min )   *minX = rangeX.min ;
        if( *maxX < rangeX.max )   *maxX = rangeX.max;
    }
    );
}


void MainWindow::onTrackerTimeUpdated(double current_time)
{
    double minX, maxX;
    getMaximumRangeX( &minX, &maxX );

    double ratio = (current_time - minX)/(double)(maxX-minX);

    double min_slider = (double)ui->horizontalSlider->minimum();
    double max_slider = (double)ui->horizontalSlider->maximum();
    int slider_value = (int)((max_slider- min_slider)* ratio) ;

    ui->horizontalSlider->setValue(slider_value);

    //------------------------
    for ( auto it = _state_publisher.begin(); it != _state_publisher.end(); it++)
    {
        it->second->updateState( &_mapped_plot_data, current_time);
    }
}

void MainWindow::onTrackerPositionUpdated(QPointF pos)
{
    onTrackerTimeUpdated( pos.x() );
    emit  trackerTimeUpdated( QPointF(pos ) );

}

void MainWindow::createTabbedDialog(PlotMatrix* first_tab, bool undoable)
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

    if( undoable ) onUndoableChange();
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
            DataLoader *loader = qobject_cast<DataLoader *>(plugin);
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

            StatePublisher *publisher = qobject_cast<StatePublisher *>(plugin);
            if (publisher)
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

            DataStreamer *streamer =  qobject_cast<DataStreamer *>(plugin);
            if (streamer)
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

    for(auto& word: words_list)
        _curvelist_widget->addItem( new QListWidgetItem(word) );

    foreach( const QString& name, words_list)
    {
        double A =  6* ((double)qrand()/(double)RAND_MAX)  - 3;
        double B =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double C =  3* ((double)qrand()/(double)RAND_MAX)  ;
        double D =  20* ((double)qrand()/(double)RAND_MAX)  ;

        PlotDataPtr plot ( new PlotData(  ) );
        plot->setName(  name.toStdString() );

        double t = 0;
        for (unsigned indx=0; indx<SIZE; indx++)
        {
            t += 0.001;
            plot->pushBack( PlotData::Point( t,  A*sin(B*t + C) + D*t*0.02 ) ) ;
        }
        _mapped_plot_data.numeric.insert( std::make_pair( name.toStdString(), plot) );
    }
    ui->horizontalSlider->setRange(0, SIZE  );
}


void MainWindow::mousePressEvent(QMouseEvent *)
{

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

    connect( this, SIGNAL(trackerTimeUpdated(QPointF)), plot->tracker(), SLOT(setPosition(QPointF)));
    connect( this, SIGNAL(trackerTimeUpdated(QPointF)), plot, SLOT( replot() ));

    connect( this, SIGNAL(activateTracker(bool)),  plot->tracker(), SLOT(setEnabled(bool)) );
    connect( this, SIGNAL(activateTracker(bool)),  plot, SLOT( replot() ));

    plot->tracker()->setEnabled(  ui->pushButtonActivateTracker->isChecked() );

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
        createTabbedDialog( NULL, false );
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

    auto items_to_remove = _curvelist_widget->findItems( curve_name );
    qDeleteAll( items_to_remove );

    emit requestRemoveCurveByName( curve_name );

    _mapped_plot_data.numeric.erase( plot_curve );

    if( _curvelist_widget->count() == 0)
    {
        ui->actionReloadData->setEnabled( false );
        ui->actionDeleteAllData->setEnabled( false );
    }
}


void MainWindow::onDeleteLoadedData()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, tr("Warning"),
                                  tr("Do you really want to remove any loaded data?\n"),
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
            _curvelist_widget->addItem( new QListWidgetItem( qname ) );
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

void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    QSlider* slider = ui->horizontalSlider;
    double ratio = (double)position / (double)(slider->maximum() -  slider->minimum() );

    double minX, maxX;
    getMaximumRangeX( &minX, &maxX);

    double posX = (maxX-minX) * ratio + minX;

    onTrackerTimeUpdated( posX );
    emit  trackerTimeUpdated( QPointF(posX,0 ) );
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
    createTabbedDialog( first_tab, true );
}


void MainWindow::updateInternalState()
{
    std::map<QString,TabbedPlotWidget*> tabbed_map;
    tabbed_map.insert( std::make_pair( QString("Main window"), _main_tabbed_widget) );

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

void MainWindow::on_pushButtonAddSubwindow_pressed()
{
    createTabbedDialog( NULL, true );
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
    }
}

void MainWindow::on_streamingSpinBox_valueChanged(int value)
{
    for (auto it = _mapped_plot_data.numeric.begin(); it != _mapped_plot_data.numeric.end(); it++ )
    {
        auto& plot = it->second;
        plot->setMaximumRangeX( value );
    }

    for (auto it = _mapped_plot_data.user_defined.begin(); it != _mapped_plot_data.user_defined.end(); it++ )
    {
        auto& plot = it->second;
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
