#include <functional>
#include <stdio.h>
#include <numeric>

#include <QApplication>
#include <QActionGroup>
#include <QCheckBox>
#include <QCommandLineParser>
#include <QDebug>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QGroupBox>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPluginLoader>
#include <QPushButton>
#include <QKeySequence>
#include <QScrollBar>
#include <QSettings>
#include <QStringListModel>
#include <QStringRef>
#include <QThread>
#include <QTextStream>
#include <QWindow>
#include <QHeaderView>

#include "mainwindow.h"
#include "curvelist_panel.h"
#include "tabbedplotwidget.h"
#include "PlotJuggler/selectlistdialog.h"
#include "PlotJuggler/plotdata.h"
#include "qwt_plot_canvas.h"
#include "transforms/function_editor.h"
#include "utils.h"

#include "ui_aboutdialog.h"
#include "ui_support_dialog.h"
#include "cheatsheet/video_cheatsheet.h"
#include "preferences_dialog.h"

MainWindow::MainWindow(const QCommandLineParser& commandline_parser, QWidget* parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
  , _undo_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Z), this)
  , _redo_shortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Z), this)
  , _fullscreen_shortcut(Qt::Key_F10, this)
  , _streaming_shortcut(QKeySequence(Qt::CTRL + Qt::Key_Space), this)
  , _playback_shotcut(Qt::Key_Space, this)
  , _minimized(false)
  , _current_streamer(nullptr)
  , _disable_undo_logging(false)
  , _tracker_time(0)
  , _tracker_param(CurveTracker::VALUE)
  , _style_directory("style_light")
{
  QLocale::setDefault(QLocale::c());  // set as default

  _test_option = commandline_parser.isSet("test");
  _autostart_publishers = commandline_parser.isSet("publish");

  _curvelist_widget = new CurveListPanel(_custom_plots, this);

  ui->setupUi(this);

  if (commandline_parser.isSet("buffer_size"))
  {
    int buffer_size = std::max(10, commandline_parser.value("buffer_size").toInt());
    ui->streamingSpinBox->setMaximum(buffer_size);
  }

  connect(this, &MainWindow::stylesheetChanged, this, &MainWindow::on_stylesheetChanged);

  connect(this, &MainWindow::stylesheetChanged, _curvelist_widget, &CurveListPanel::on_stylesheetChanged);

  connect(_curvelist_widget, &CurveListPanel::hiddenItemsChanged, this, &MainWindow::onUpdateLeftTableValues);

  connect(_curvelist_widget, &CurveListPanel::deleteCurves, this, &MainWindow::onDeleteMultipleCurves);

  connect(_curvelist_widget, &CurveListPanel::createMathPlot, this, &MainWindow::on_addMathPlot);

  connect(_curvelist_widget, &CurveListPanel::editMathPlot, this, &MainWindow::on_editMathPlot);

  connect(_curvelist_widget, &CurveListPanel::refreshMathPlot, this, &MainWindow::on_refreshMathPlot);

  connect(ui->timeSlider, &RealSlider::realValueChanged, this, &MainWindow::onTimeSlider_valueChanged);

  connect(ui->playbackRate, &QDoubleSpinBox::editingFinished, this, [this]() { ui->playbackRate->clearFocus(); });
  connect(ui->playbackStep, &QDoubleSpinBox::editingFinished, this, [this]() { ui->playbackStep->clearFocus(); });

  _main_tabbed_widget = new TabbedPlotWidget("Main Window", this, nullptr, _mapped_plot_data, this);

  connect(this, &MainWindow::stylesheetChanged, _main_tabbed_widget, &TabbedPlotWidget::on_stylesheetChanged);

  ui->plottingLayout->insertWidget(0, _main_tabbed_widget, 1);
  ui->leftLayout->addWidget(_curvelist_widget);

  ui->splitter->setCollapsible(0, true);
  ui->splitter->setStretchFactor(0, 2);
  ui->splitter->setStretchFactor(1, 6);

  connect(ui->splitter, SIGNAL(splitterMoved(int, int)), SLOT(on_splitterMoved(int, int)));

  initializeActions();
  initializePlugins(QCoreApplication::applicationDirPath());
  initializePlugins("/usr/local/PlotJuggler/plugins");

  _undo_timer.start();

  // save initial state
  onUndoableChange();

  _replot_timer = new QTimer(this);
  _replot_timer->setInterval(40);
  connect(_replot_timer, &QTimer::timeout, this, [this]() { updateDataAndReplot(false); });

  _publish_timer = new QTimer(this);
  _publish_timer->setInterval(20);
  connect(_publish_timer, &QTimer::timeout, this, &MainWindow::onPlaybackLoop);

  ui->menuFile->setToolTipsVisible(true);
  ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
  ui->streamingLabel->setHidden(true);
  ui->streamingSpinBox->setHidden(true);

  this->setMenuBar(ui->menuBar);
  ui->menuBar->setNativeMenuBar(false);

  if (_test_option)
  {
    connect(ui->actionLoadDummyData, &QAction::triggered, this, &MainWindow::buildDummyData);
    buildDummyData();
  }
  else
  {
    ui->actionLoadDummyData->setVisible(false);
  }

  bool file_loaded = false;
  if (commandline_parser.isSet("datafile"))
  {
    QStringList datafiles = commandline_parser.values("datafile");
    file_loaded = loadDataFromFiles(datafiles);
  }
  if (commandline_parser.isSet("layout"))
  {
    loadLayoutFromFile(commandline_parser.value("layout"));
  }

  QSettings settings;
  restoreGeometry(settings.value("MainWindow.geometry").toByteArray());

  bool activate_grid = settings.value("MainWindow.activateGrid", false).toBool();
  ui->pushButtonActivateGrid->setChecked(activate_grid);

  int streaming_buffer_value = settings.value("MainWindow.streamingBufferValue", 5).toInt();
  ui->streamingSpinBox->setValue(streaming_buffer_value);

  bool datetime_display = settings.value("MainWindow.dateTimeDisplay", false).toBool();
  ui->pushButtonUseDateTime->setChecked(datetime_display);

  bool remove_time_offset = settings.value("MainWindow.removeTimeOffset", true).toBool();
  ui->pushButtonRemoveTimeOffset->setChecked(remove_time_offset);

  ui->widgetOptions->setVisible(ui->pushButtonOptions->isChecked());

  //----------------------------------------------------------
  QIcon trackerIconA, trackerIconB, trackerIconC;

  trackerIconA.addFile(QStringLiteral(":/style_light/line_tracker.png"), QSize(36, 36));
  trackerIconB.addFile(QStringLiteral(":/style_light/line_tracker_1.png"), QSize(36, 36));
  trackerIconC.addFile(QStringLiteral(":/style_light/line_tracker_a.png"), QSize(36, 36));

  _tracker_button_icons[CurveTracker::LINE_ONLY] = trackerIconA;
  _tracker_button_icons[CurveTracker::VALUE] = trackerIconB;
  _tracker_button_icons[CurveTracker::VALUE_NAME] = trackerIconC;

  int tracker_setting = settings.value("MainWindow.timeTrackerSetting", (int)CurveTracker::VALUE).toInt();
  _tracker_param = static_cast<CurveTracker::Parameter>(tracker_setting);

  ui->pushButtonTimeTracker->setIcon(_tracker_button_icons[_tracker_param]);

  forEachWidget([&](PlotWidget* plot) { plot->configureTracker(_tracker_param); });

  _style_directory = settings.value("Preferences::theme", "style_light").toString();
  emit stylesheetChanged(_style_directory);
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::onUndoableChange()
{
  if (_disable_undo_logging)
    return;

  int elapsed_ms = _undo_timer.restart();

  // overwrite the previous
  if (elapsed_ms < 100)
  {
    if (_undo_states.empty() == false)
      _undo_states.pop_back();
  }

  while (_undo_states.size() >= 100)
    _undo_states.pop_front();
  _undo_states.push_back(xmlSaveState());
  _redo_states.clear();
  //    qDebug() << "undo " << _undo_states.size();
}

void MainWindow::onRedoInvoked()
{
  _disable_undo_logging = true;
  if (_redo_states.size() > 0)
  {
    QDomDocument state_document = _redo_states.back();
    while (_undo_states.size() >= 100)
      _undo_states.pop_front();
    _undo_states.push_back(state_document);
    _redo_states.pop_back();

    xmlLoadState(state_document);
  }
  //    qDebug() << "undo " << _undo_states.size();
  _disable_undo_logging = false;
}

void MainWindow::onUndoInvoked()
{
  _disable_undo_logging = true;
  if (_undo_states.size() > 1)
  {
    QDomDocument state_document = _undo_states.back();
    while (_redo_states.size() >= 100)
      _redo_states.pop_front();
    _redo_states.push_back(state_document);
    _undo_states.pop_back();
    state_document = _undo_states.back();

    xmlLoadState(state_document);
  }
  //    qDebug() << "undo " << _undo_states.size();
  _disable_undo_logging = false;
}

void MainWindow::onUpdateLeftTableValues()
{
  _curvelist_widget->update2ndColumnValues(_tracker_time, &_mapped_plot_data.numeric);
}

void MainWindow::onTrackerMovedFromWidget(QPointF relative_pos)
{
  _tracker_time = relative_pos.x() + _time_offset.get();

  auto prev = ui->timeSlider->blockSignals(true);
  ui->timeSlider->setRealValue(_tracker_time);
  ui->timeSlider->blockSignals(prev);

  onTrackerTimeUpdated(_tracker_time, true);
}

void MainWindow::onTimeSlider_valueChanged(double abs_time)
{
  _tracker_time = abs_time;
  onTrackerTimeUpdated(_tracker_time, true);
}

void MainWindow::onTrackerTimeUpdated(double absolute_time, bool do_replot)
{
  updatedDisplayTime();
  onUpdateLeftTableValues();

  for (auto& it : _state_publisher)
  {
    it.second->updateState(absolute_time);
  }

  forEachWidget([&](PlotWidget* plot) {
    plot->setTrackerPosition(_tracker_time);
    if (do_replot)
    {
      plot->replot();
    }
  });
}

void MainWindow::createTabbedDialog(QString suggest_win_name, PlotMatrix* first_tab)
{
  if (suggest_win_name.isEmpty())
  {
    for (size_t i = 0; i <= TabbedPlotWidget::instances().size(); i++)
    {
      suggest_win_name = QString("Window%1").arg(i);
      TabbedPlotWidget* tw = TabbedPlotWidget::instance(suggest_win_name);
      if (tw == nullptr)
      {
        break;
      }
    }
  }

  SubWindow* window = new SubWindow(suggest_win_name, first_tab, _mapped_plot_data, this);

  connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(onFloatingWindowDestroyed(QObject*)));
  connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(onUndoableChange()));

  window->tabbedWidget()->setStreamingMode(isStreamingActive());

  window->setAttribute(Qt::WA_DeleteOnClose, true);
  window->show();
  window->activateWindow();
  window->raise();

  if (this->signalsBlocked() == false)
    onUndoableChange();
}

void MainWindow::initializeActions()
{
  _undo_shortcut.setContext(Qt::ApplicationShortcut);
  _redo_shortcut.setContext(Qt::ApplicationShortcut);
  _fullscreen_shortcut.setContext(Qt::ApplicationShortcut);

  connect(&_undo_shortcut, &QShortcut::activated, this, &MainWindow::onUndoInvoked);
  connect(&_redo_shortcut, &QShortcut::activated, this, &MainWindow::onRedoInvoked);
  connect(&_streaming_shortcut, &QShortcut::activated, this, &MainWindow::on_streamingToggled);
  connect(&_playback_shotcut, &QShortcut::activated, ui->pushButtonPlay, &QPushButton::toggle);
  connect(&_fullscreen_shortcut, &QShortcut::activated, this, &MainWindow::onActionFullscreenTriggered);

  QShortcut* open_menu_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_F), this);
  connect(open_menu_shortcut, &QShortcut::activated,
          [this]() { ui->menuFile->exec(ui->menuBar->mapToGlobal(QPoint(0, 25))); });

  QShortcut* open_streaming_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_S), this);
  connect(open_streaming_shortcut, &QShortcut::activated,
          [this]() { ui->menuStreaming->exec(ui->menuBar->mapToGlobal(QPoint(50, 25))); });

  QShortcut* open_publish_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_P), this);
  connect(open_publish_shortcut, &QShortcut::activated,
          [this]() { ui->menuPublishers->exec(ui->menuBar->mapToGlobal(QPoint(140, 25))); });

  QShortcut* open_help_shortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_H), this);
  connect(open_help_shortcut, &QShortcut::activated,
          [this]() { ui->menuHelp->exec(ui->menuBar->mapToGlobal(QPoint(230, 25))); });

  //---------------------------------------------

  QSettings settings;
  updateRecentDataMenu(settings.value("MainWindow.recentlyLoadedDatafile").toStringList());
  updateRecentLayoutMenu(settings.value("MainWindow.recentlyLoadedLayout").toStringList());
}

void MainWindow::initializePlugins(QString directory_name)
{
  static std::set<QString> loaded_plugins;

  QDir pluginsDir(directory_name);

  for (const QString& filename : pluginsDir.entryList(QDir::Files))
  {
    QFileInfo fileinfo(filename);
    if (fileinfo.suffix() != "so" && fileinfo.suffix() != "dll" && fileinfo.suffix() != "dylib")
    {
      continue;
    }

    if (loaded_plugins.find(filename) != loaded_plugins.end())
    {
      continue;
    }

    QPluginLoader pluginLoader(pluginsDir.absoluteFilePath(filename), this);

    QObject* plugin = pluginLoader.instance();
    if (plugin)
    {
      DataLoader* loader = qobject_cast<DataLoader*>(plugin);
      StatePublisher* publisher = qobject_cast<StatePublisher*>(plugin);
      DataStreamer* streamer = qobject_cast<DataStreamer*>(plugin);

      QString plugin_name;
      if (loader)
        plugin_name = loader->name();
      if (publisher)
        plugin_name = publisher->name();
      if (streamer)
        plugin_name = streamer->name();

      if (loaded_plugins.find(plugin_name) == loaded_plugins.end())
      {
        loaded_plugins.insert(plugin_name);
      }
      else
      {
        QMessageBox::warning(this, tr("Warning"),
                             tr("Trying to load twice a plugin with name [%1].\n"
                                "Only the first will be loaded.")
                                 .arg(plugin_name));
        continue;
      }

      if (loader)
      {
        qDebug() << filename << ": is a DataLoader plugin";
        if (!_test_option && loader->isDebugPlugin())
        {
          qDebug() << filename << "...but will be ignored unless the argument -t is used.";
        }
        else
        {
          _data_loader.insert(std::make_pair(plugin_name, loader));
        }
      }
      else if (publisher)
      {
        publisher->setDataMap(&_mapped_plot_data);
        qDebug() << filename << ": is a StatePublisher plugin";
        if (!_test_option && publisher->isDebugPlugin())
        {
          qDebug() << filename << "...but will be ignored unless the argument -t is used.";
        }
        else
        {
          ui->menuPublishers->setEnabled(true);

          _state_publisher.insert(std::make_pair(plugin_name, publisher));
          QAction* activatePublisher = new QAction(tr("Start: ") + plugin_name, this);
          activatePublisher->setProperty("starter_button", true);
          activatePublisher->setCheckable(true);
          activatePublisher->setChecked(false);

          ui->menuPublishers->addSeparator();
          ui->menuPublishers->addSection(plugin_name);
          ui->menuPublishers->addAction(activatePublisher);
          publisher->setParentMenu(ui->menuPublishers, activatePublisher);

          connect(activatePublisher, &QAction::toggled, this,
                  [=](bool enable) { publisher->setEnabled(enable); });

          connect(publisher, &StatePublisher::connectionClosed, this,
                  [=]() { activatePublisher->setChecked(false); });
        }
      }
      else if (streamer)
      {
        qDebug() << filename << ": is a DataStreamer plugin";
        if (!_test_option && streamer->isDebugPlugin())
        {
          qDebug() << filename << "...but will be ignored unless the argument -t is used.";
        }
        else
        {
          _data_streamer.insert(std::make_pair(plugin_name, streamer));

          QAction* startStreamer = new QAction(QString("Start: ") + plugin_name, this);
          ui->menuStreaming->setEnabled(true);
          ui->menuStreaming->addAction(startStreamer);

          streamer->addActionsToParentMenu(ui->menuStreaming);
          ui->menuStreaming->addSeparator();

          connect(startStreamer, &QAction::triggered, this, [=]() { on_actionStartStreaming(plugin_name); });

          connect(streamer, &DataStreamer::connectionClosed, ui->actionStopStreaming, &QAction::trigger);

          connect(streamer, &DataStreamer::clearBuffers, this, &MainWindow::on_actionClearBuffer_triggered);
        }
      }
    }
    else
    {
      if (pluginLoader.errorString().contains("is not an ELF object") == false)
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
  QStringList words_list;
  words_list << "world/siam"
             << "world/tre"
             << "walk/piccoli"
             << "walk/porcellin"
             << "fly/high/mai"
             << "fly/high/nessun"
             << "fly/low/ci"
             << "fly/low/dividera"
             << "data_1"
             << "data_2"
             << "data_3"
             << "data_10";

  for (int i = 0; i < 100; i++)
  {
    words_list.append(QString("data_vect/%1").arg(count++));
  }

  for (const QString& name : words_list)
  {
    double A = 6 * ((double)qrand() / (double)RAND_MAX) - 3;
    double B = 3 * ((double)qrand() / (double)RAND_MAX);
    double C = 3 * ((double)qrand() / (double)RAND_MAX);
    double D = 20 * ((double)qrand() / (double)RAND_MAX);

    auto it = datamap.addNumeric(name.toStdString());
    PlotData& plot = it->second;

    double t = 0;
    for (unsigned indx = 0; indx < SIZE; indx++)
    {
      t += 0.01;
      plot.pushBack(PlotData::Point(t + 35, A * sin(B * t + C) + D * t * 0.02));
    }
  }

  PlotData& sin_plot = datamap.addNumeric("_sin")->second;
  PlotData& cos_plot = datamap.addNumeric("_cos")->second;

  double t = 0;
  for (unsigned indx = 0; indx < SIZE; indx++)
  {
    t += 0.01;
    sin_plot.pushBack(PlotData::Point(t + 20, sin(t * 0.4)));
    cos_plot.pushBack(PlotData::Point(t + 20, cos(t * 0.4)));
  }

  importPlotDataMap(datamap, true);
}

void MainWindow::on_splitterMoved(int, int)
{
  QList<int> sizes = ui->splitter->sizes();
  int maxLeftWidth = _curvelist_widget->maximumWidth();
  int totalWidth = sizes[0] + sizes[1];

  if (sizes[0] > maxLeftWidth)
  {
    sizes[0] = maxLeftWidth;
    sizes[1] = totalWidth - maxLeftWidth;
    ui->splitter->setSizes(sizes);
  }
}

void MainWindow::resizeEvent(QResizeEvent*)
{
  on_splitterMoved(0, 0);
}

void MainWindow::showEvent(QShowEvent* ev)
{
  QMainWindow::showEvent(ev);

  static bool first = true;
  if (first)
  {
    first = false;
    QSettings settings;
    int splitter_width = settings.value("MainWindow.splitterWidth", 200).toInt();
    QList<int> splitter_sizes = ui->splitter->sizes();
    int tot_splitter_width = splitter_sizes[0] + splitter_sizes[1];
    splitter_sizes[0] = splitter_width;
    splitter_sizes[1] = tot_splitter_width - splitter_width;
    ui->splitter->setSizes(splitter_sizes);
  }
}

void MainWindow::onPlotAdded(PlotWidget* plot)
{
  connect(plot, &PlotWidget::undoableChange, this, &MainWindow::onUndoableChange);

  connect(plot, &PlotWidget::trackerMoved, this, &MainWindow::onTrackerMovedFromWidget);

  connect(plot, &PlotWidget::swapWidgetsRequested, this, &MainWindow::onSwapPlots);

  connect(this, &MainWindow::requestRemoveCurveByName, plot, &PlotWidget::removeCurve);

  connect(plot, &PlotWidget::curveListChanged, this, [this]() {
    updateTimeOffset();
    updateTimeSlider();
  });

  connect(&_time_offset, SIGNAL(valueChanged(double)), plot, SLOT(on_changeTimeOffset(double)));

  connect(ui->pushButtonUseDateTime, &QPushButton::toggled, plot, &PlotWidget::on_changeDateTimeScale);

  connect(plot, &PlotWidget::curvesDropped, _curvelist_widget, &CurveListPanel::clearSelections);

  plot->on_changeTimeOffset(_time_offset.get());
  plot->on_changeDateTimeScale(ui->pushButtonUseDateTime->isChecked());
  plot->activateGrid(ui->pushButtonActivateGrid->isChecked());
  plot->enableTracker(!isStreamingActive());
  plot->configureTracker(_tracker_param);
}

void MainWindow::onPlotMatrixAdded(PlotMatrix* matrix)
{
  connect(matrix, &PlotMatrix::plotAdded, this, &MainWindow::onPlotAdded);
  connect(matrix, &PlotMatrix::undoableChange, this, &MainWindow::onUndoableChange);
}

QDomDocument MainWindow::xmlSaveState() const
{
  QDomDocument doc;
  QDomProcessingInstruction instr = doc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-8'");

  doc.appendChild(instr);

  QDomElement root = doc.createElement("root");

  for (auto& it : TabbedPlotWidget::instances())
  {
    QDomElement tabbed_area = it.second->xmlSaveState(doc);
    root.appendChild(tabbed_area);
  }

  doc.appendChild(root);

  QDomElement relative_time = doc.createElement("use_relative_time_offset");
  relative_time.setAttribute("enabled", ui->pushButtonRemoveTimeOffset->isChecked());
  root.appendChild(relative_time);

  return doc;
}

void MainWindow::checkAllCurvesFromLayout(const QDomElement& root)
{
  std::set<std::string> curves;

  for (QDomElement tw = root.firstChildElement("tabbed_widget"); !tw.isNull(); tw = tw.nextSiblingElement("tabbed_"
                                                                                                          "widget"))
  {
    for (QDomElement pm = tw.firstChildElement("plotmatrix"); !pm.isNull(); pm = pm.nextSiblingElement("plotmatrix"))
    {
      for (QDomElement pl = pm.firstChildElement("plot"); !pl.isNull(); pl = pl.nextSiblingElement("plot"))
      {
        QDomElement tran_elem = pl.firstChildElement("transform");
        std::string trans = tran_elem.attribute("value").toStdString();
        bool is_XY_plot = (trans == "XYPlot");

        for (QDomElement cv = pl.firstChildElement("curve"); !cv.isNull(); cv = cv.nextSiblingElement("curve"))
        {
          if (is_XY_plot)
          {
            curves.insert(cv.attribute("curve_x").toStdString());
            curves.insert(cv.attribute("curve_y").toStdString());
          }
          else
          {
            curves.insert(cv.attribute("name").toStdString());
          }
        }
      }
    }
  }

  std::vector<std::string> missing_curves;

  for (auto& curve_name : curves)
  {
    if (_mapped_plot_data.numeric.count(curve_name) == 0)
    {
      missing_curves.push_back(curve_name);
    }
  }
  if (missing_curves.size() > 0)
  {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Warning");
    msgBox.setText(tr("One or more timeseries in the layout haven't been loaded yet\n"
                      "What do you want to do?"));

    QPushButton* buttonRemove = msgBox.addButton(tr("Remove curves from plots"), QMessageBox::RejectRole);
    QPushButton* buttonPlaceholder = msgBox.addButton(tr("Create empty placeholders"), QMessageBox::YesRole);
    msgBox.setDefaultButton(buttonPlaceholder);
    msgBox.exec();
    if (msgBox.clickedButton() == buttonPlaceholder)
    {
      for (auto& name : missing_curves)
      {
        _curvelist_widget->addCurve(QString::fromStdString(name));
        _mapped_plot_data.addNumeric(name);
      }
      _curvelist_widget->refreshColumns();
    }
  }
}

bool MainWindow::xmlLoadState(QDomDocument state_document)
{
  QDomElement root = state_document.namedItem("root").toElement();
  if (root.isNull())
  {
    qWarning() << "No <root> element found at the top-level of the XML file!";
    return false;
  }

  size_t num_floating = 0;
  std::map<QString, QDomElement> tabbed_widgets_with_name;

  for (QDomElement tw = root.firstChildElement("tabbed_widget"); tw.isNull() == false; tw = tw.nextSiblingElement("tabb"
                                                                                                                  "ed_"
                                                                                                                  "widg"
                                                                                                                  "et"))
  {
    if (tw.attribute("parent") != ("main_window"))
    {
      num_floating++;
    }
    tabbed_widgets_with_name[tw.attribute("name")] = tw;
  }

  // add if missing
  for (const auto& it : tabbed_widgets_with_name)
  {
    if (TabbedPlotWidget::instance(it.first) == nullptr)
    {
      createTabbedDialog(it.first, nullptr);
    }
  }

  // remove those which don't share list of names
  for (const auto& it : TabbedPlotWidget::instances())
  {
    if (tabbed_widgets_with_name.count(it.first) == 0)
    {
      it.second->deleteLater();
    }
  }

  //-----------------------------------------------------
  checkAllCurvesFromLayout(root);
  //-----------------------------------------------------

  for (QDomElement tw = root.firstChildElement("tabbed_widget"); tw.isNull() == false; tw = tw.nextSiblingElement("tabb"
                                                                                                                  "ed_"
                                                                                                                  "widg"
                                                                                                                  "et"))
  {
    TabbedPlotWidget* tabwidget = TabbedPlotWidget::instance(tw.attribute("name"));
    tabwidget->xmlLoadState(tw);
  }

  QDomElement relative_time = root.firstChildElement("use_relative_time_offset");
  if (!relative_time.isNull())
  {
    bool remove_offset = (relative_time.attribute("enabled") == QString("1"));
    ui->pushButtonRemoveTimeOffset->setChecked(remove_offset);
  }
  return true;
}

void MainWindow::onDeleteMultipleCurves(const std::vector<std::string>& curve_names)
{
  for (const auto& curve_name : curve_names)
  {
    auto plot_curve = _mapped_plot_data.numeric.find(curve_name);
    if (plot_curve == _mapped_plot_data.numeric.end())
    {
      continue;
    }

    emit requestRemoveCurveByName(curve_name);
    _mapped_plot_data.numeric.erase(plot_curve);

    auto custom_it = _custom_plots.find(curve_name);
    if (custom_it != _custom_plots.end())
    {
      _custom_plots.erase(custom_it);
    }

    _curvelist_widget->removeCurve(curve_name);
  }

  forEachWidget([](PlotWidget* plot) { plot->replot(); });
}

void MainWindow::updateRecentDataMenu(QStringList new_filenames)
{
  QMenu* menu = ui->menuRecentData;

  QAction* separator = nullptr;
  QStringList prev_filenames;
  for (QAction* action : menu->actions())
  {
    if (action->isSeparator())
    {
      separator = action;
      break;
    }
    if (new_filenames.contains(action->text()) == false)
    {
      prev_filenames.push_back(action->text());
    }
    menu->removeAction(action);
  }

  new_filenames.append(prev_filenames);
  while (new_filenames.size() > 10)
  {
    new_filenames.removeLast();
  }

  for (const auto& filename : new_filenames)
  {
    QAction* action = new QAction(filename, nullptr);
    connect(action, &QAction::triggered, this, [this, filename] { loadDataFromFiles({ filename }); });
    menu->insertAction(separator, action);
  }

  QSettings settings;
  settings.setValue("MainWindow.recentlyLoadedDatafile", new_filenames);
  menu->setEnabled(new_filenames.size() > 0);
}

void MainWindow::updateRecentLayoutMenu(QStringList new_filenames)
{
  QMenu* menu = ui->menuRecentLayout;

  QAction* separator = nullptr;
  QStringList prev_filenames;
  for (QAction* action : menu->actions())
  {
    if (action->isSeparator())
    {
      separator = action;
      break;
    }
    if (new_filenames.contains(action->text()) == false)
    {
      prev_filenames.push_back(action->text());
    }
    menu->removeAction(action);
  }

  new_filenames.append(prev_filenames);
  while (new_filenames.size() > 10)
  {
    new_filenames.removeLast();
  }

  for (const auto& filename : new_filenames)
  {
    QAction* action = new QAction(filename, nullptr);
    connect(action, &QAction::triggered, this, [this, filename] {
      if (this->loadLayoutFromFile(filename))
      {
        updateRecentLayoutMenu({ filename });
      }
    });
    menu->insertAction(separator, action);
  }

  QSettings settings;
  settings.setValue("MainWindow.recentlyLoadedLayout", new_filenames);
  menu->setEnabled(new_filenames.size() > 0);
}

void MainWindow::deleteAllData()
{
  forEachWidget([](PlotWidget* plot) { plot->removeAllCurves(); });

  _mapped_plot_data.numeric.clear();
  _mapped_plot_data.user_defined.clear();
  _custom_plots.clear();
  _curvelist_widget->clear();
  _loaded_datafiles.clear();

  bool stopped = false;
  for (QAction* action : ui->menuPublishers->actions())
  {
    auto is_start_button = action->property("starter_button");
    if (is_start_button.isValid() && is_start_button.toBool() && action->isChecked())
    {
      action->setChecked(false);
      stopped = true;
    }
  }

  if (stopped)
  {
    QMessageBox::warning(this, "State publishers stopped",
                         "All the state publishers have been stopped because old data has been deleted.");
  }
}

template <typename T>
void importPlotDataMapHelper(std::unordered_map<std::string, T>& source,
                             std::unordered_map<std::string, T>& destination, bool delete_older)
{
  for (auto& it : source)
  {
    const std::string& name = it.first;
    T& source_plot = it.second;
    auto plot_with_same_name = destination.find(name);

    // this is a new plot
    if (plot_with_same_name == destination.end())
    {
      plot_with_same_name =
          destination.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name)).first;
    }
    T& destination_plot = plot_with_same_name->second;

    if (delete_older)
    {
      double max_range_x = destination_plot.maximumRangeX();
      destination_plot.swapData(source_plot);
      destination_plot.setMaximumRangeX(max_range_x);  // just in case
    }
    else
    {
      for (size_t i = 0; i < source_plot.size(); i++)
      {
        destination_plot.pushBack(source_plot.at(i));
      }
    }
    source_plot.clear();
  }
}

void MainWindow::importPlotDataMap(PlotDataMapRef& new_data, bool remove_old)
{
  if (new_data.user_defined.empty() && new_data.numeric.empty())
  {
    return;
  }

  if (remove_old)
  {
    std::vector<std::string> old_plots_to_delete;

    for (auto& it : _mapped_plot_data.numeric)
    {
      // timeseries in old but not in new
      if (new_data.numeric.count(it.first) == 0)
      {
        old_plots_to_delete.push_back(it.first);
      }
    }

    if (!old_plots_to_delete.empty())
    {
      QMessageBox::StandardButton reply;
      reply = QMessageBox::question(this, tr("Warning"), tr("Do you want to remove the previously loaded data?\n"),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
      if (reply == QMessageBox::Yes)
      {
        onDeleteMultipleCurves(old_plots_to_delete);
      }
    }
  }

  bool curvelist_modified = false;
  for (auto& it : new_data.numeric)
  {
    const std::string& name = it.first;
    if (it.second.size() > 0 && _mapped_plot_data.numeric.count(name) == 0)
    {
      _curvelist_widget->addCurve(QString::fromStdString(name));
      curvelist_modified = true;
    }
  }

  importPlotDataMapHelper(new_data.numeric, _mapped_plot_data.numeric, remove_old);
  importPlotDataMapHelper(new_data.user_defined, _mapped_plot_data.user_defined, remove_old);

  if (curvelist_modified)
  {
    _curvelist_widget->refreshColumns();
  }
}

bool MainWindow::isStreamingActive() const
{
  return ui->pushButtonStreaming->isChecked() && _current_streamer;
}

bool MainWindow::loadDataFromFiles(QStringList filenames)
{
  if (filenames.size() > 1)
  {
    static bool show_me = true;

    QMessageBox msgbox;
    msgbox.setWindowTitle("Loading multiple files");
    msgbox.setText("You are loading multiple files at once. A prefix will be automatically added to the name of the "
                   "timeseries.\n\n"
                   "This is an experimental feature. Publishers will not work as you may expect.");
    msgbox.addButton(QMessageBox::Ok);
    //    QCheckBox *cb = new QCheckBox("Don't show this again");
    //    msgbox.setCheckBox(cb);
    //    connect(cb, &QCheckBox::stateChanged, this, [this, cb , &show_me]() {  show_me = !cb->isChecked(); } );
    msgbox.exec();
  }

  char prefix_ch = 'A';
  QStringList loaded_filenames;

  for (int i = 0; i < filenames.size(); i++)
  {
    FileLoadInfo info;
    info.filename = filenames[i];
    if (filenames.size() > 1)
    {
      info.prefix = prefix_ch;
    }

    if (loadDataFromFile(info))
    {
      loaded_filenames.push_back(filenames[i]);
      prefix_ch++;
    }
  }
  if (loaded_filenames.size() > 0)
  {
    updateRecentDataMenu(loaded_filenames);
    return true;
  }
  return false;
}

bool MainWindow::loadDataFromFile(const FileLoadInfo& info)
{
  ui->pushButtonPlay->setChecked(false);

  const QString extension = QFileInfo(info.filename).suffix().toLower();

  typedef std::map<QString, DataLoader*>::iterator MapIterator;

  std::vector<MapIterator> compatible_loaders;

  for (MapIterator it = _data_loader.begin(); it != _data_loader.end(); ++it)
  {
    DataLoader* data_loader = it->second;
    std::vector<const char*> extensions = data_loader->compatibleFileExtensions();

    for (auto& ext : extensions)
    {
      if (extension == QString(ext).toLower())
      {
        compatible_loaders.push_back(it);
        break;
      }
    }
  }

  DataLoader* dataloader = nullptr;

  if (compatible_loaders.size() == 1)
  {
    dataloader = compatible_loaders.front()->second;
  }
  else
  {
    static QString last_plugin_name_used;

    QStringList names;
    for (auto& cl : compatible_loaders)
    {
      const auto& name = cl->first;

      if (name == last_plugin_name_used)
      {
        names.push_front(name);
      }
      else
      {
        names.push_back(name);
      }
    }

    bool ok;
    QString plugin_name = QInputDialog::getItem(this, tr("QInputDialog::getItem()"), tr("Select the loader to use:"),
                                                names, 0, false, &ok);
    if (ok && !plugin_name.isEmpty())
    {
      dataloader = _data_loader[plugin_name];
      last_plugin_name_used = plugin_name;
    }
  }

  if (dataloader)
  {
    QFile file(info.filename);

    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
      QMessageBox::warning(this, tr("Datafile"),
                           tr("Cannot read file %1:\n%2.").arg(info.filename).arg(file.errorString()));
      return false;
    }
    file.close();

    try
    {
      PlotDataMapRef mapped_data;
      FileLoadInfo new_info = info;

      if (dataloader->readDataFromFile(&new_info, mapped_data))
      {
        AddPrefixToPlotData(info.prefix.toStdString(), mapped_data.numeric);

        importPlotDataMap(mapped_data, true);

        QDomElement plugin_elem = dataloader->xmlSaveState(new_info.plugin_config);
        new_info.plugin_config.appendChild(plugin_elem);

        bool duplicate = false;

        // substitute an old item of _loaded_datafiles or push_back another item.
        for (auto& prev_loaded : _loaded_datafiles)
        {
          if (prev_loaded.filename == new_info.filename && prev_loaded.prefix == new_info.prefix)
          {
            prev_loaded = new_info;
            duplicate = true;
            break;
          }
        }

        if (!duplicate)
        {
          _loaded_datafiles.push_back(new_info);
        }
      }
    }
    catch (std::exception& ex)
    {
      QMessageBox::warning(
          this, tr("Exception from the plugin"),
          tr("The plugin [%1] thrown the following exception: \n\n %3\n").arg(dataloader->name()).arg(ex.what()));
      return false;
    }
  }
  else
  {
    QMessageBox::warning(this, tr("Error"),
                         tr("Cannot read files with extension %1.\n No plugin can handle that!\n").arg(info.filename));
  }
  _curvelist_widget->updateFilter();
  updateDataAndReplot(true);
  ui->timeSlider->setRealValue(ui->timeSlider->getMinimum());

  return true;
}

QString MainWindow::styleDirectory() const
{
  return _style_directory;
}

void MainWindow::on_actionStartStreaming(QString streamer_name)
{
  if (_current_streamer)
  {
    _current_streamer->shutdown();
    _current_streamer = nullptr;
  }

  if (_data_streamer.empty())
  {
    qDebug() << "Error, no streamer loaded";
    return;
  }

  if (_data_streamer.size() == 1)
  {
    _current_streamer = _data_streamer.begin()->second;
  }
  else if (_data_streamer.size() > 1)
  {
    auto it = _data_streamer.find(streamer_name);
    if (it != _data_streamer.end())
    {
      _current_streamer = it->second;
    }
    else
    {
      qDebug() << "Error. The streamer " << streamer_name << " can't be loaded";
      return;
    }
  }

  bool started = false;
  try
  {
    // TODO data sources
    started = _current_streamer && _current_streamer->start(nullptr);
  }
  catch (std::runtime_error& err)
  {
    QMessageBox::warning(this, tr("Exception from the plugin"),
                         tr("The plugin thrown the following exception: \n\n %1\n").arg(err.what()));
    return;
  }
  if (started)
  {
    {
      std::lock_guard<std::mutex> lock(_current_streamer->mutex());
      importPlotDataMap(_current_streamer->dataMap(), true);
    }

    for (auto& action : ui->menuStreaming->actions())
    {
      action->setEnabled(false);
    }
    ui->actionClearBuffer->setEnabled(true);

    ui->actionStopStreaming->setEnabled(true);
    ui->actionDeleteAllData->setToolTip("Stop streaming to be able to delete the data");

    ui->pushButtonStreaming->setEnabled(true);
    ui->pushButtonStreaming->setChecked(true);
    ui->pushButtonRemoveTimeOffset->setEnabled(false);

    on_streamingSpinBox_valueChanged(ui->streamingSpinBox->value());
  }
  else
  {
    qDebug() << "Failed to launch the streamer";
  }
}

void MainWindow::on_stylesheetChanged(QString style_dir)
{
  QFile styleFile(tr("://%1/stylesheet.qss").arg(style_dir));
  styleFile.open(QFile::ReadOnly);
  dynamic_cast<QApplication*>(QCoreApplication::instance())->setStyleSheet(styleFile.readAll());

  ui->pushButtonOptions->setIcon(QIcon(tr(":/%1/settings_cog.png").arg(style_dir)));
  // ui->pushButtonTimeTracker->setIcon(QIcon(tr(":/%1/line_tracker_1.png").arg(style_dir)));
  ui->playbackLoop->setIcon(QIcon(tr(":/%1/loop.png").arg(style_dir)));
  ui->pushButtonPlay->setIcon(QIcon(tr(":/%1/play_arrow.png").arg(style_dir)));
  ui->pushButtonUseDateTime->setIcon(QIcon(tr(":/%1/datetime.png").arg(style_dir)));
  ui->pushButtonActivateGrid->setIcon(QIcon(tr(":/%1/grid.png").arg(style_dir)));
  ui->pushButtonRatio->setIcon(QIcon(tr(":/%1/ratio.png").arg(style_dir)));
  ui->actionClearRecentData->setIcon(QIcon(tr(":/%1/clean_pane.png").arg(style_dir)));
  ui->actionClearRecentLayout->setIcon(QIcon(tr(":/%1/clean_pane.png").arg(style_dir)));
}

void MainWindow::loadPluginState(const QDomElement& root)
{
  QDomElement plugins = root.firstChildElement("Plugins");

  for (QDomElement plugin_elem = plugins.firstChildElement(); plugin_elem.isNull() == false;
       plugin_elem = plugin_elem.nextSiblingElement())
  {
    const QString plugin_name = plugin_elem.attribute("ID");

    if (plugin_elem.nodeName() != "plugin" || plugin_name.isEmpty())
    {
      QMessageBox::warning(this, tr("Error loading Plugin State from Layout"),
                           tr("The method xmlSaveState() must return a node like this <plugin ID=\"PluginName\" "));
    }

    if (_data_loader.find(plugin_name) != _data_loader.end())
    {
      _data_loader[plugin_name]->xmlLoadState(plugin_elem);
    }
    if (_data_streamer.find(plugin_name) != _data_streamer.end())
    {
      _data_streamer[plugin_name]->xmlLoadState(plugin_elem);
    }
    if (_state_publisher.find(plugin_name) != _state_publisher.end())
    {
      StatePublisher* publisher = _state_publisher[plugin_name];
      publisher->xmlLoadState(plugin_elem);

      if (_autostart_publishers && plugin_elem.attribute("status") == "active")
      {
        publisher->setEnabled(true);
      }
    }
  }
}

QDomElement MainWindow::savePluginState(QDomDocument& doc)
{
  QDomElement list_plugins = doc.createElement("Plugins");

  auto CheckValidFormat = [this](const QString& expected_name, const QDomElement& elem) {
    if (elem.nodeName() != "plugin" || elem.attribute("ID") != expected_name)
    {
      QMessageBox::warning(this, tr("Error saving Plugin State to Layout"),
                           tr("The method xmlSaveState() returned\n<plugin ID=\"%1\">\ninstead of\n<plugin ID=\"%2\">")
                               .arg(elem.attribute("ID"))
                               .arg(expected_name));
    }
  };

  for (auto& it : _data_loader)
  {
    const DataLoader* dataloader = it.second;
    QDomElement plugin_elem = dataloader->xmlSaveState(doc);
    if (!plugin_elem.isNull())
    {
      list_plugins.appendChild(plugin_elem);
      CheckValidFormat(it.first, plugin_elem);
    }
  }

  for (auto& it : _data_streamer)
  {
    const DataStreamer* datastreamer = it.second;
    QDomElement plugin_elem = datastreamer->xmlSaveState(doc);
    if (!plugin_elem.isNull())
    {
      list_plugins.appendChild(plugin_elem);
      CheckValidFormat(it.first, plugin_elem);
    }
  }

  for (auto& it : _state_publisher)
  {
    const StatePublisher* state_publisher = it.second;
    QDomElement plugin_elem = state_publisher->xmlSaveState(doc);
    if (!plugin_elem.isNull())
    {
      list_plugins.appendChild(plugin_elem);
      CheckValidFormat(it.first, plugin_elem);
    }

    plugin_elem.setAttribute("status", state_publisher->enabled() ? "active" : "idle");
  }
  return list_plugins;
}

std::tuple<double, double, int> MainWindow::calculateVisibleRangeX()
{
  // find min max time
  double min_time = std::numeric_limits<double>::max();
  double max_time = -std::numeric_limits<double>::max();
  int max_steps = 0;

  forEachWidget([&](PlotWidget* widget) {
    for (auto& it : widget->curveList())
    {
      const auto& curve_name = it.first;

      auto plot_it = _mapped_plot_data.numeric.find(curve_name);
      if (plot_it == _mapped_plot_data.numeric.end())
      {
        continue;  // FIXME?
      }
      const auto& data = plot_it->second;
      if (data.size() >= 1)
      {
        const double t0 = data.front().x;
        const double t1 = data.back().x;
        min_time = std::min(min_time, t0);
        max_time = std::max(max_time, t1);
        max_steps = std::max(max_steps, (int)data.size());
      }
    }
  });

  // needed if all the plots are empty
  if (max_steps == 0 || max_time < min_time)
  {
    for (const auto& it : _mapped_plot_data.numeric)
    {
      const PlotData& data = it.second;
      if (data.size() >= 1)
      {
        const double t0 = data.front().x;
        const double t1 = data.back().x;
        min_time = std::min(min_time, t0);
        max_time = std::max(max_time, t1);
        max_steps = std::max(max_steps, (int)data.size());
      }
    }
  }

  // last opportunity. Everything else failed
  if (max_steps == 0 || max_time < min_time)
  {
    min_time = 0.0;
    max_time = 1.0;
    max_steps = 1;
  }
  return std::tuple<double, double, int>(min_time, max_time, max_steps);
}

static const QString LAYOUT_VERSION = "2.3.8";

bool MainWindow::loadLayoutFromFile(QString filename)
{
  QSettings settings;

  QFile file(filename);
  if (!file.open(QFile::ReadOnly | QFile::Text))
  {
    QMessageBox::warning(this, tr("Layout"), tr("Cannot read file %1:\n%2.").arg(filename).arg(file.errorString()));
    return false;
  }

  QString errorStr;
  int errorLine, errorColumn;

  QDomDocument domDocument;

  if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
  {
    QMessageBox::information(window(), tr("XML Layout"),
                             tr("Parse error at line %1:\n%2").arg(errorLine).arg(errorStr));
    return false;
  }

  //-------------------------------------------------
  // refresh plugins
  QDomElement root = domDocument.namedItem("root").toElement();

  if (!root.hasAttribute("version") || root.attribute("version") != LAYOUT_VERSION)
  {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Obsolate Layout version");
    msgBox.setText(tr("This Layout ID is not supported [%1].\nThis version of PlotJuggler use Layout ID [%2]")
                       .arg(root.attribute("version"))
                       .arg(LAYOUT_VERSION));

    msgBox.setStandardButtons(QMessageBox::Abort);
    QPushButton* continueButton = msgBox.addButton(tr("Continue anyway"), QMessageBox::ActionRole);
    msgBox.setDefaultButton(continueButton);

    int ret = msgBox.exec();
    if (ret == QMessageBox::Abort)
    {
      return false;
    }
  }

  loadPluginState(root);
  //-------------------------------------------------
  QDomElement previously_loaded_datafile = root.firstChildElement("previouslyLoaded_Datafiles");

  QDomElement datafile_elem = previously_loaded_datafile.firstChildElement("fileInfo");
  while (!datafile_elem.isNull())
  {
    FileLoadInfo info;
    info.filename = datafile_elem.attribute("filename");
    info.prefix = datafile_elem.attribute("prefix");

    QDomElement datasources_elem = datafile_elem.firstChildElement("selected_datasources");
    QString topics_list = datasources_elem.attribute("value");
    info.selected_datasources = topics_list.split(";", QString::SkipEmptyParts);

    auto plugin_elem = datafile_elem.firstChildElement("plugin");
    info.plugin_config.appendChild(info.plugin_config.importNode(plugin_elem, true));

    loadDataFromFile(info);
    datafile_elem = datafile_elem.nextSiblingElement("fileInfo");
  }

  QDomElement previousl_streamer = root.firstChildElement("previouslyLoaded_Streamer");
  if (!previousl_streamer.isNull())
  {
    QString streamer_name = previousl_streamer.attribute("name");

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Start Streaming?");
    msgBox.setText(tr("Start the previously used streaming plugin?\n\n %1 \n\n").arg(streamer_name));
    QPushButton* yes = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);
    QPushButton* no = msgBox.addButton(tr("No"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(yes);
    msgBox.exec();

    if (msgBox.clickedButton() == yes)
    {
      if (_data_streamer.count(streamer_name) != 0)
      {
        on_actionStartStreaming(streamer_name);
      }
      else
      {
        QMessageBox::warning(this, tr("Error Loading Streamer"),
                             tr("The streamer named %1 can not be loaded.").arg(streamer_name));
      }
    }
  }
  //-------------------------------------------------
  // autostart_publishers
  QDomElement plugins = root.firstChildElement("Plugins");

  if (!plugins.isNull() && _autostart_publishers)
  {
    for (QDomElement plugin_elem = plugins.firstChildElement(); plugin_elem.isNull() == false;
         plugin_elem = plugin_elem.nextSiblingElement())
    {
      const QString plugin_name = plugin_elem.nodeName();
      if (_state_publisher.find(plugin_name) != _state_publisher.end())
      {
        StatePublisher* publisher = _state_publisher[plugin_name];

        if (plugin_elem.attribute("status") == "active")
        {
          publisher->setEnabled(true);
        }
      }
    }
  }
  //-------------------------------------------------
  auto custom_equations = root.firstChildElement("customMathEquations");

  try
  {
    if (!custom_equations.isNull())
    {
      for (QDomElement custom_eq = custom_equations.firstChildElement("snippet"); custom_eq.isNull() == false;
           custom_eq = custom_eq.nextSiblingElement("snippet"))
      {
        CustomPlotPtr new_custom_plot = CustomFunction::createFromXML(custom_eq);
        const auto& name = new_custom_plot->name();
        _custom_plots[name] = new_custom_plot;
        new_custom_plot->calculateAndAdd(_mapped_plot_data);
        _curvelist_widget->addCustom(QString::fromStdString(name));
      }
      _curvelist_widget->refreshColumns();
    }
  }
  catch (std::runtime_error& err)
  {
    QMessageBox::warning(this, tr("Exception"), tr("Failed to refresh a customMathEquation \n\n %1\n").arg(err.what()));
  }

  QByteArray snippets_saved_xml = settings.value("AddCustomPlotDialog.savedXML", QByteArray()).toByteArray();

  auto snippets_element = root.firstChildElement("snippets");
  if (!snippets_element.isNull())
  {
    auto snippets_previous = GetSnippetsFromXML(snippets_saved_xml);
    auto snippets_layout = GetSnippetsFromXML(snippets_element);

    bool snippets_are_different = false;
    for (const auto& snippet_it : snippets_layout)
    {
      auto prev_it = snippets_previous.find(snippet_it.first);

      if (prev_it == snippets_previous.end() || prev_it->second.equation != snippet_it.second.equation ||
          prev_it->second.globalVars != snippet_it.second.globalVars)
      {
        snippets_are_different = true;
        break;
      }
    }

    if (snippets_are_different)
    {
      QMessageBox msgBox(this);
      msgBox.setWindowTitle("Overwrite custom transforms?");
      msgBox.setText("Your layour file contains a set of custom transforms different from "
                     "the last one you used.\nant to load these transformations?");
      msgBox.addButton(QMessageBox::No);
      msgBox.addButton(QMessageBox::Yes);
      msgBox.setDefaultButton(QMessageBox::Yes);

      if (msgBox.exec() == QMessageBox::Yes)
      {
        for (const auto& snippet_it : snippets_layout)
        {
          snippets_previous[snippet_it.first] = snippet_it.second;
        }
        QDomDocument doc;
        auto snippets_root_element = ExportSnippets(snippets_previous, doc);
        doc.appendChild(snippets_root_element);
        settings.setValue("AddCustomPlotDialog.savedXML", doc.toByteArray(2));
      }
    }
  }

  ///--------------------------------------------------

  xmlLoadState(domDocument);

  forEachWidget([&](PlotWidget* plot) { plot->zoomOut(false); });

  _undo_states.clear();
  _undo_states.push_back(domDocument);
  return true;
}

void MainWindow::on_tabbedAreaDestroyed(QObject* object)
{
  this->setFocus();
}

void MainWindow::onFloatingWindowDestroyed(QObject* object)
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
  createTabbedDialog(QString(), first_tab);
}

void MainWindow::forEachWidget(std::function<void(PlotWidget*, PlotMatrix*, int, int)> operation)
{
  auto func = [&](QTabWidget* tabs) {
    for (int t = 0; t < tabs->count(); t++)
    {
      PlotMatrix* matrix = static_cast<PlotMatrix*>(tabs->widget(t));

      for (unsigned row = 0; row < matrix->rowsCount(); row++)
      {
        for (unsigned col = 0; col < matrix->colsCount(); col++)
        {
          PlotWidget* plot = matrix->plotAt(row, col);
          operation(plot, matrix, row, col);
        }
      }
    }
  };

  for (const auto& it : TabbedPlotWidget::instances())
  {
    func(it.second->tabWidget());
  }
}

void MainWindow::forEachWidget(std::function<void(PlotWidget*)> op)
{
  forEachWidget([&](PlotWidget* plot, PlotMatrix*, int, int) { op(plot); });
}

void MainWindow::updateTimeSlider()
{
  auto range = calculateVisibleRangeX();

  ui->timeSlider->setLimits(std::get<0>(range), std::get<1>(range), std::get<2>(range));

  _tracker_time = std::max(_tracker_time, ui->timeSlider->getMinimum());
  _tracker_time = std::min(_tracker_time, ui->timeSlider->getMaximum());
}

void MainWindow::updateTimeOffset()
{
  auto range = calculateVisibleRangeX();
  double min_time = std::get<0>(range);

  const bool remove_offset = ui->pushButtonRemoveTimeOffset->isChecked();
  if (remove_offset && min_time != std::numeric_limits<double>::max())
  {
    _time_offset.set(min_time);
  }
  else
  {
    _time_offset.set(0.0);
  }
}

void MainWindow::onSwapPlots(PlotWidget* source, PlotWidget* destination)
{
  if (!source || !destination)
    return;

  PlotMatrix* src_matrix = nullptr;
  PlotMatrix* dst_matrix = nullptr;
  QPoint src_pos;
  QPoint dst_pos;

  forEachWidget([&](PlotWidget* plot, PlotMatrix* matrix, int row, int col) {
    if (plot == source)
    {
      src_matrix = matrix;
      src_pos.setX(row);
      src_pos.setY(col);
    }
    else if (plot == destination)
    {
      dst_matrix = matrix;
      dst_pos.setX(row);
      dst_pos.setY(col);
    }
  });

  if (src_matrix && dst_matrix)
  {
    src_matrix->gridLayout()->removeWidget(source);
    dst_matrix->gridLayout()->removeWidget(destination);

    src_matrix->gridLayout()->addWidget(destination, src_pos.x(), src_pos.y());
    dst_matrix->gridLayout()->addWidget(source, dst_pos.x(), dst_pos.y());

    src_matrix->updateLayout();
    if (src_matrix != dst_matrix)
    {
      dst_matrix->updateLayout();
    }
    source->changeBackgroundColor(Qt::white);
    destination->changeBackgroundColor(Qt::white);
  }
  onUndoableChange();
}

void MainWindow::on_pushButtonStreaming_toggled(bool streaming)
{
  if (!_current_streamer)
  {
    streaming = false;
  }

  ui->pushButtonRemoveTimeOffset->setEnabled(!streaming);

  if (streaming)
  {
    ui->horizontalSpacer->changeSize(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->pushButtonStreaming->setText("Streaming ON");
  }
  else
  {
    _replot_timer->stop();
    ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->pushButtonStreaming->setText("Streaming OFF");
  }
  ui->streamingLabel->setHidden(!streaming);
  ui->streamingSpinBox->setHidden(!streaming);
  ui->timeSlider->setHidden(streaming);
  ui->pushButtonPlay->setHidden(streaming);

  if (streaming && ui->pushButtonPlay->isChecked())
  {
    ui->pushButtonPlay->setChecked(false);
  }

  forEachWidget([&](PlotWidget* plot) { plot->enableTracker(!streaming); });

  emit activateStreamingMode(streaming);

  if (_current_streamer && streaming)
  {
    _replot_timer->start();
    updateTimeOffset();
  }
  else
  {
    updateDataAndReplot(true);
    onUndoableChange();
  }
}

void MainWindow::on_streamingToggled()
{
  if (ui->pushButtonStreaming->isEnabled())
  {
    bool streaming = ui->pushButtonStreaming->isChecked();
    ui->pushButtonStreaming->setChecked(!streaming);
  }
  else
  {
    if (ui->pushButtonPlay->isEnabled())
    {
      bool playing = ui->pushButtonPlay->isChecked();
      ui->pushButtonPlay->setChecked(!playing);
    }
  }
}

void MainWindow::updateDataAndReplot(bool replot_hidden_tabs)
{
  if (_current_streamer)
  {
    std::vector<QString> curvelist_added;
    {
      std::lock_guard<std::mutex> lock(_current_streamer->mutex());
      curvelist_added = MoveData(_current_streamer->dataMap(), _mapped_plot_data);
    }

    for (const auto& str : curvelist_added)
    {
      _curvelist_widget->addCurve(str);
    }

    if (curvelist_added.size() > 0)
    {
      _curvelist_widget->refreshColumns();
    }
  }

  for (auto& custom_it : _custom_plots)
  {
    auto* dst_plot = &_mapped_plot_data.numeric.at(custom_it.first);
    custom_it.second->calculate(_mapped_plot_data, dst_plot);
  }

  const bool is_streaming_active = isStreamingActive();

  forEachWidget([is_streaming_active](PlotWidget* plot) {
    plot->updateCurves();
    plot->setZoomEnabled(!is_streaming_active);
  });

  //--------------------------------
  // trigger again the execution of this callback if steaming == true
  if (is_streaming_active)
  {
    auto range = calculateVisibleRangeX();
    double max_time = std::get<1>(range);
    _tracker_time = max_time;

    onTrackerTimeUpdated(_tracker_time, false);
  }
  else
  {
    updateTimeOffset();
    updateTimeSlider();
  }
  //--------------------------------
  for (const auto& it : TabbedPlotWidget::instances())
  {
    if (replot_hidden_tabs)
    {
      QTabWidget* tabs = it.second->tabWidget();
      for (int index = 0; index < tabs->count(); index++)
      {
        PlotMatrix* matrix = static_cast<PlotMatrix*>(tabs->widget(index));
        matrix->maximumZoomOut();
      }
    }
    else
    {
      PlotMatrix* matrix = it.second->currentTab();
      matrix->maximumZoomOut();  // includes replot
    }
  }
}

void MainWindow::on_streamingSpinBox_valueChanged(int value)
{
  double real_value = value;

  if (isStreamingActive() == false)
  {
    return;
  }

  for (auto& it : _mapped_plot_data.numeric)
  {
    it.second.setMaximumRangeX(real_value);
  }

  for (auto& it : _mapped_plot_data.user_defined)
  {
    it.second.setMaximumRangeX(real_value);
  }

  if (_current_streamer)
  {
    _current_streamer->setMaximumRange(real_value);
  }
}

void MainWindow::on_actionStopStreaming_triggered()
{
  ui->pushButtonStreaming->setChecked(false);
  ui->pushButtonStreaming->setEnabled(false);
  _replot_timer->stop();
  _current_streamer->shutdown();
  _current_streamer = nullptr;

  for (auto& action : ui->menuStreaming->actions())
  {
    action->setEnabled(true);
  }
  ui->actionStopStreaming->setEnabled(false);

  if (!_mapped_plot_data.numeric.empty())
  {
    ui->actionDeleteAllData->setToolTip("");
  }

  // reset this.
  for (auto& it : _mapped_plot_data.numeric)
  {
    it.second.setMaximumRangeX(std::numeric_limits<double>::max());
  }
  for (auto& it : _mapped_plot_data.user_defined)
  {
    it.second.setMaximumRangeX(std::numeric_limits<double>::max());
  }
}

void MainWindow::on_actionExit_triggered()
{
  this->close();
}

void MainWindow::on_pushButtonRemoveTimeOffset_toggled(bool)
{
  updateTimeOffset();
  updatedDisplayTime();

  forEachWidget([](PlotWidget* plot) { plot->replot(); });

  if (this->signalsBlocked() == false)
    onUndoableChange();
}

void MainWindow::on_pushButtonOptions_toggled(bool checked)
{
  ui->widgetOptions->setVisible(checked);
}

void MainWindow::updatedDisplayTime()
{
  QLineEdit* timeLine = ui->displayTime;
  const double relative_time = _tracker_time - _time_offset.get();
  if (ui->pushButtonUseDateTime->isChecked())
  {
    if (ui->pushButtonRemoveTimeOffset->isChecked())
    {
      QTime time = QTime::fromMSecsSinceStartOfDay(std::round(relative_time * 1000.0));
      timeLine->setText(time.toString("HH:mm::ss.zzz"));
    }
    else
    {
      QDateTime datetime = QDateTime::fromMSecsSinceEpoch(std::round(_tracker_time * 1000.0));
      timeLine->setText(datetime.toString("[yyyy MMM dd] HH:mm::ss.zzz"));
    }
  }
  else
  {
    timeLine->setText(QString::number(relative_time, 'f', 3));
  }

  QFontMetrics fm(timeLine->font());
  int width = fm.width(timeLine->text()) + 10;
  timeLine->setFixedWidth(std::max(100, width));
}

void MainWindow::on_pushButtonActivateGrid_toggled(bool checked)
{
  forEachWidget([checked](PlotWidget* plot) {
    plot->activateGrid(checked);
    plot->replot();
  });
}

void MainWindow::on_pushButtonRatio_toggled(bool checked)
{
  forEachWidget([checked](PlotWidget* plot) {
    plot->setConstantRatioXY(checked);
    plot->replot();
  });
}

void MainWindow::on_pushButtonPlay_toggled(bool checked)
{
  if (checked)
  {
    _publish_timer->start();
    _prev_publish_time = QDateTime::currentDateTime();
  }
  else
  {
    _publish_timer->stop();
  }
}

void MainWindow::on_actionClearBuffer_triggered()
{
  for (auto& it : _mapped_plot_data.numeric)
  {
    it.second.clear();
  }

  for (auto& it : _mapped_plot_data.user_defined)
  {
    it.second.clear();
  }

  for (auto& it : _custom_plots)
  {
    it.second->clear();
  }

  forEachWidget([](PlotWidget* plot) {
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
  if (_tracker_param == CurveTracker::LINE_ONLY)
  {
    _tracker_param = CurveTracker::VALUE;
  }
  else if (_tracker_param == CurveTracker::VALUE)
  {
    _tracker_param = CurveTracker::VALUE_NAME;
  }
  else if (_tracker_param == CurveTracker::VALUE_NAME)
  {
    _tracker_param = CurveTracker::LINE_ONLY;
  }
  ui->pushButtonTimeTracker->setIcon(_tracker_button_icons[_tracker_param]);

  forEachWidget([&](PlotWidget* plot) {
    plot->configureTracker(_tracker_param);
    plot->replot();
  });
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  _replot_timer->stop();
  _publish_timer->stop();

  if (_current_streamer)
  {
    _current_streamer->shutdown();
    _current_streamer = nullptr;
  }
  QSettings settings;
  settings.setValue("MainWindow.geometry", saveGeometry());
  settings.setValue("MainWindow.activateGrid", ui->pushButtonActivateGrid->isChecked());
  settings.setValue("MainWindow.streamingBufferValue", ui->streamingSpinBox->value());
  settings.setValue("MainWindow.removeTimeOffset", ui->pushButtonRemoveTimeOffset->isChecked());
  settings.setValue("MainWindow.dateTimeDisplay", ui->pushButtonUseDateTime->isChecked());
  settings.setValue("MainWindow.timeTrackerSetting", (int)_tracker_param);
  settings.setValue("MainWindow.splitterWidth", ui->splitter->sizes()[0]);

  // clean up all the plugins
  for (auto& it : _data_loader)
  {
    delete it.second;
  }
  for (auto& it : _state_publisher)
  {
    delete it.second;
  }
  for (auto& it : _data_streamer)
  {
    delete it.second;
  }
}

void MainWindow::on_addMathPlot(const std::string& linked_name)
{
  addOrEditMathPlot(linked_name, false);
}

void MainWindow::on_editMathPlot(const std::string& plot_name)
{
  addOrEditMathPlot(plot_name, true);
}

void MainWindow::on_refreshMathPlot(const std::string& plot_name)
{
  try
  {
    auto custom_it = _custom_plots.find(plot_name);
    if (custom_it == _custom_plots.end())
    {
      qWarning("failed to find custom equation");
      return;
    }
    CustomPlotPtr ce = custom_it->second;

    ce->calculateAndAdd(_mapped_plot_data);

    onUpdateLeftTableValues();
    updateDataAndReplot(true);
  }
  catch (const std::runtime_error& e)
  {
    QMessageBox::critical(this, "error", "Failed to refresh data : " + QString::fromStdString(e.what()));
  }
}

void MainWindow::addOrEditMathPlot(const std::string& name, bool modifying)
{
  AddCustomPlotDialog dialog(_mapped_plot_data, _custom_plots, this);

  if (!modifying)
  {
    dialog.setLinkedPlotName(QString::fromStdString(name));
    dialog.setEditorMode(AddCustomPlotDialog::FUNCTION_OR_TIMESERIES);
  }
  else
  {
    dialog.setEditorMode(AddCustomPlotDialog::TIMESERIES_ONLY);

    auto custom_it = _custom_plots.find(name);
    if (custom_it == _custom_plots.end())
    {
      qWarning("failed to find custom equation");
      return;
    }
    dialog.editExistingPlot(custom_it->second);
  }

  if (dialog.exec() == QDialog::Accepted)
  {
    if (modifying)
    {
      // clear already existing data first
      auto data_it = _mapped_plot_data.numeric.find(name);
      if (data_it != _mapped_plot_data.numeric.end())
      {
        data_it->second.clear();
      }
    }

    const QString& qplot_name = dialog.getName();
    std::string plot_name = qplot_name.toStdString();
    CustomPlotPtr eq = dialog.getCustomPlotData();

    try
    {
      eq->calculateAndAdd(_mapped_plot_data);
    }
    catch (std::exception& ex)
    {
      QMessageBox::warning(this, tr("Warning"),
                           tr("Failed to create the custom timeseries. Error:\n\n%1").arg(ex.what()));

      return;
    }

    // keep data for reference
    auto custom_it = _custom_plots.find(plot_name);
    if (custom_it == _custom_plots.end())
    {
      _custom_plots.insert({ plot_name, eq });
    }
    else
    {
      custom_it->second = eq;
      modifying = true;
    }

    if (!modifying)
    {
      _curvelist_widget->addCustom(qplot_name);
    }
    onUpdateLeftTableValues();

    if (modifying)
    {
      updateDataAndReplot(true);
    }
  }
}

void MainWindow::onPlaybackLoop()
{
  qint64 delta_ms = (QDateTime::currentMSecsSinceEpoch() - _prev_publish_time.toMSecsSinceEpoch());
  _prev_publish_time = QDateTime::currentDateTime();
  delta_ms = std::max((qint64)_publish_timer->interval(), delta_ms);

  _tracker_time += delta_ms * 0.001 * ui->playbackRate->value();
  if (_tracker_time >= ui->timeSlider->getMaximum())
  {
    if (!ui->playbackLoop->isChecked())
    {
      ui->pushButtonPlay->setChecked(false);
    }
    _tracker_time = ui->timeSlider->getMinimum();
  }
  //////////////////
  auto prev = ui->timeSlider->blockSignals(true);
  ui->timeSlider->setRealValue(_tracker_time);
  ui->timeSlider->blockSignals(prev);

  //////////////////
  updatedDisplayTime();
  onUpdateLeftTableValues();

  for (auto& it : _state_publisher)
  {
    it.second->play(_tracker_time);
  }

  forEachWidget([&](PlotWidget* plot) {
    plot->setTrackerPosition(_tracker_time);
    plot->replot();
  });
}

void MainWindow::on_actionReportBug_triggered()
{
  QDesktopServices::openUrl(QUrl("https://github.com/facontidavide/PlotJuggler/issues"));
}

void MainWindow::on_actionShare_the_love_triggered()
{
  QDesktopServices::openUrl(QUrl("https://twitter.com/intent/tweet?hashtags=PlotJuggler"));
}

void MainWindow::on_actionAbout_triggered()
{
  QDialog* dialog = new QDialog(this);
  auto ui = new Ui::AboutDialog();
  ui->setupUi(dialog);

  ui->label_version->setText(QApplication::applicationVersion());
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  dialog->exec();
}

void MainWindow::on_actionCheatsheet_triggered()
{
  QSettings settings;

  HelpVideo* dialog = new HelpVideo(this);
  dialog->restoreGeometry(settings.value("Cheatsheet.geometry").toByteArray());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();

  connect(dialog, &QDialog::finished, this, [this, dialog]() {
    QSettings settings;
    settings.setValue("Cheatsheet.geometry", dialog->saveGeometry());
  });
}

void MainWindow::on_actionSupportPlotJuggler_triggered()
{
  QDialog* dialog = new QDialog(this);
  auto ui = new Ui::SupportDialog();
  ui->setupUi(dialog);

  dialog->setAttribute(Qt::WA_DeleteOnClose);

  dialog->exec();
}

void MainWindow::on_actionSaveAllPlotTabs_triggered()
{
  QSettings settings;
  QString directory_path = settings.value("MainWindow.saveAllPlotTabs", QDir::currentPath()).toString();
  // Get destination folder
  QFileDialog saveDialog(this);
  saveDialog.setDirectory(directory_path);
  saveDialog.setFileMode(QFileDialog::FileMode::Directory);
  saveDialog.setAcceptMode(QFileDialog::AcceptSave);
  saveDialog.exec();

  uint image_number = 1;
  if (saveDialog.result() == QDialog::Accepted && !saveDialog.selectedFiles().empty())
  {
    // Save Plots
    QString directory = saveDialog.selectedFiles().first();
    settings.setValue("MainWindow.saveAllPlotTabs", directory);

    QStringList file_names;
    QStringList existing_files;
    QDateTime current_date_time(QDateTime::currentDateTime());
    QString current_date_time_name(current_date_time.toString("yyyy-MM-dd_HH-mm-ss"));
    for (const auto& it : TabbedPlotWidget::instances())
    {
      auto tab_widget = it.second->tabWidget();
      for (int i = 0; i < tab_widget->count(); i++)
      {
        PlotMatrix* matrix = static_cast<PlotMatrix*>(tab_widget->widget(i));
        QString name = QString("%1/%2_%3_%4.png")
                           .arg(directory)
                           .arg(current_date_time_name)
                           .arg(image_number, 2, 10, QLatin1Char('0'))
                           .arg(matrix->name());
        file_names.push_back(name);
        image_number++;

        QFileInfo check_file(file_names.back());
        if (check_file.exists() && check_file.isFile())
        {
          existing_files.push_back(name);
        }
      }
    }
    if (existing_files.isEmpty() == false)
    {
      QMessageBox msgBox;
      msgBox.setText("One or more files will be overwritten. ant to continue?");
      QString all_files;
      for (const auto& str : existing_files)
      {
        all_files.push_back("\n");
        all_files.append(str);
      }
      msgBox.setInformativeText(all_files);
      msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);

      if (msgBox.exec() != QMessageBox::Ok)
      {
        return;
      }
    }

    image_number = 0;
    for (const auto& it : TabbedPlotWidget::instances())
    {
      auto tab_widget = it.second->tabWidget();
      for (int i = 0; i < tab_widget->count(); i++)
      {
        PlotMatrix* matrix = static_cast<PlotMatrix*>(tab_widget->widget(i));
        TabbedPlotWidget::saveTabImage(file_names[image_number], matrix);
        image_number++;
      }
    }
  }
}

void MainWindow::on_actionLoadData_triggered()
{
  if (_data_loader.empty())
  {
    QMessageBox::warning(this, tr("Warning"), tr("No plugin was loaded to process a data file\n"));
    return;
  }

  QSettings settings;

  QString file_extension_filter;

  std::set<QString> extensions;

  for (auto& it : _data_loader)
  {
    DataLoader* loader = it.second;
    for (QString extension : loader->compatibleFileExtensions())
    {
      extensions.insert(extension.toLower());
    }
  }

  for (const auto& it : extensions)
  {
    file_extension_filter.append(QString(" *.") + it);
  }

  QString directory_path = settings.value("MainWindow.lastDatafileDirectory", QDir::currentPath()).toString();

  QFileDialog loadDialog(this);
  loadDialog.setFileMode(QFileDialog::ExistingFiles);
  loadDialog.setViewMode(QFileDialog::Detail);
  loadDialog.setNameFilter(file_extension_filter);
  loadDialog.setDirectory(directory_path);

  QStringList fileNames;
  if (loadDialog.exec())
  {
    fileNames = loadDialog.selectedFiles();
  }

  if (fileNames.isEmpty())
  {
    return;
  }

  directory_path = QFileInfo(fileNames[0]).absolutePath();
  settings.setValue("MainWindow.lastDatafileDirectory", directory_path);

  if (loadDataFromFiles(fileNames))
  {
    updateRecentDataMenu(fileNames);
  }
}

void MainWindow::on_actionLoadLayout_triggered()
{
  QSettings settings;

  QString directory_path = settings.value("MainWindow.lastLayoutDirectory", QDir::currentPath()).toString();
  QString filename = QFileDialog::getOpenFileName(this, "Open Layout", directory_path, "*.xml");
  if (filename.isEmpty())
  {
    return;
  }

  if (loadLayoutFromFile(filename))
  {
    updateRecentLayoutMenu({ filename });
  }

  directory_path = QFileInfo(filename).absolutePath();
  settings.setValue("MainWindow.lastLayoutDirectory", directory_path);
}

void MainWindow::on_actionSaveLayout_triggered()

{
  QDomDocument doc = xmlSaveState();

  QSettings settings;

  QString directory_path = settings.value("MainWindow.lastLayoutDirectory", QDir::currentPath()).toString();

  QFileDialog saveDialog(this);
  saveDialog.setOption(QFileDialog::DontUseNativeDialog, true);

  QGridLayout* save_layout = static_cast<QGridLayout*>(saveDialog.layout());

  QFrame* frame = new QFrame;
  frame->setFrameStyle(QFrame::Box | QFrame::Plain);
  frame->setLineWidth(1);

  QVBoxLayout* vbox = new QVBoxLayout;
  QLabel* title = new QLabel("Save Layout options");
  QFrame* separator = new QFrame;
  separator->setFrameStyle(QFrame::HLine | QFrame::Plain);

  auto checkbox_datasource = new QCheckBox("Save data source");
  checkbox_datasource->setToolTip("ant the layout to remember the source of your data,\n"
                                  "i.e. the Datafile used or the Streaming Plugin loaded ?");
  checkbox_datasource->setFocusPolicy(Qt::NoFocus);
  checkbox_datasource->setChecked(settings.value("MainWindow.saveLayoutDataSource", true).toBool());

  auto checkbox_snippets = new QCheckBox("Save custom transformations");
  checkbox_snippets->setToolTip("Do you want the layout to save the custom transformations?");
  checkbox_snippets->setFocusPolicy(Qt::NoFocus);
  checkbox_snippets->setChecked(settings.value("MainWindow.saveLayoutSnippets", true).toBool());

  vbox->addWidget(title);
  vbox->addWidget(separator);
  vbox->addWidget(checkbox_datasource);
  vbox->addWidget(checkbox_snippets);
  frame->setLayout(vbox);

  int rows = save_layout->rowCount();
  int col = save_layout->columnCount();
  save_layout->addWidget(frame, 0, col, rows, 1, Qt::AlignTop);

  saveDialog.setAcceptMode(QFileDialog::AcceptSave);
  saveDialog.setDefaultSuffix("xml");
  saveDialog.setNameFilter("XML (*.xml)");
  saveDialog.setDirectory(directory_path);
  saveDialog.exec();

  if (saveDialog.result() != QDialog::Accepted || saveDialog.selectedFiles().empty())
  {
    return;
  }

  QString fileName = saveDialog.selectedFiles().first();

  if (fileName.isEmpty())
  {
    return;
  }

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("MainWindow.lastLayoutDirectory", directory_path);
  settings.setValue("MainWindow.saveLayoutDataSource", checkbox_datasource->isChecked());
  settings.setValue("MainWindow.saveLayoutSnippets", checkbox_snippets->isChecked());

  QDomElement root = doc.namedItem("root").toElement();
  root.setAttribute("version", LAYOUT_VERSION);

  root.appendChild(doc.createComment(" - - - - - - - - - - - - - - "));

  root.appendChild(doc.createComment(" - - - - - - - - - - - - - - "));

  root.appendChild(savePluginState(doc));

  root.appendChild(doc.createComment(" - - - - - - - - - - - - - - "));

  if (checkbox_datasource->isChecked())
  {
    QDomElement loaded_list = doc.createElement("previouslyLoaded_Datafiles");

    for (const auto& loaded : _loaded_datafiles)
    {
      QDomElement file_elem = doc.createElement("fileInfo");
      file_elem.setAttribute("filename", loaded.filename);
      file_elem.setAttribute("prefix", loaded.prefix);

      QDomElement datasources_elem = doc.createElement("selected_datasources");
      QString topics_list = loaded.selected_datasources.join(";");
      datasources_elem.setAttribute("value", topics_list);
      file_elem.appendChild(datasources_elem);

      file_elem.appendChild(loaded.plugin_config.firstChild());
      loaded_list.appendChild(file_elem);
    }
    root.appendChild(loaded_list);

    if (_current_streamer)
    {
      QDomElement loaded_streamer = doc.createElement("previouslyLoaded_Streamer");
      QString streamer_name = _current_streamer->name();
      loaded_streamer.setAttribute("name", streamer_name);
      root.appendChild(loaded_streamer);
    }
  }
  //-----------------------------------
  root.appendChild(doc.createComment(" - - - - - - - - - - - - - - "));
  if (checkbox_snippets->isChecked())
  {
    QDomElement custom_equations = doc.createElement("customMathEquations");
    for (const auto& custom_it : _custom_plots)
    {
      const auto& custom_plot = custom_it.second;
      custom_equations.appendChild(custom_plot->xmlSaveState(doc));
    }
    root.appendChild(custom_equations);

    QByteArray snippets_xml_text = settings.value("AddCustomPlotDialog.savedXML", QByteArray()).toByteArray();
    auto snipped_saved = GetSnippetsFromXML(snippets_xml_text);
    auto snippets_root = ExportSnippets(snipped_saved, doc);
    root.appendChild(snippets_root);
  }
  root.appendChild(doc.createComment(" - - - - - - - - - - - - - - "));
  //------------------------------------
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly))
  {
    QTextStream stream(&file);
    stream << doc.toString() << endl;
  }
}

void MainWindow::onActionFullscreenTriggered()
{
  static bool first_call = true;
  if (first_call && !_minimized)
  {
    first_call = false;
    QMessageBox::information(this, "Remember!", "Press F10 to switch back to the normal view");
  }

  _minimized = !_minimized;

  ui->leftFrame->setVisible(!_minimized);
  ui->widgetOptions->setVisible(!_minimized && ui->pushButtonOptions->isChecked());
  ui->widgetTimescale->setVisible(!_minimized);
  ui->menuBar->setVisible(!_minimized);

  for (auto& it : TabbedPlotWidget::instances())
  {
    it.second->setControlsVisible(!_minimized);
  }
}

void MainWindow::on_actionLoadDummyData_triggered()
{
  buildDummyData();
}

void MainWindow::on_actionFunctionEditor_triggered()
{
  AddCustomPlotDialog dialog(_mapped_plot_data, _custom_plots, this);
  dialog.setEditorMode(AddCustomPlotDialog::FUNCTION_ONLY);
  dialog.exec();
}

void MainWindow::on_actionClearRecentData_triggered()
{
  QMenu* menu = ui->menuRecentData;
  for (QAction* action : menu->actions())
  {
    if (action->isSeparator())
    {
      break;
    }
    menu->removeAction(action);
  }
  menu->setEnabled(false);
  QSettings settings;
  settings.setValue("MainWindow.recentlyLoadedDatafile", {});
}

void MainWindow::on_actionClearRecentLayout_triggered()
{
  QMenu* menu = ui->menuRecentLayout;
  for (QAction* action : menu->actions())
  {
    if (action->isSeparator())
    {
      break;
    }
    menu->removeAction(action);
  }
  menu->setEnabled(false);
  QSettings settings;
  settings.setValue("MainWindow.recentlyLoadedLayout", {});
}

void MainWindow::on_actionDeleteAllData_triggered()
{
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Warning. Can't be undone.");
  msgBox.setText(tr("Do you want to remove the previously loaded data?\n"));
  msgBox.addButton(QMessageBox::No);
  msgBox.addButton(QMessageBox::Yes);
  msgBox.setDefaultButton(QMessageBox::Yes);
  //  QPushButton* buttonPlaceholder = msgBox.addButton(tr("Keep empty placeholders"), QMessageBox::NoRole);
  auto reply = msgBox.exec();

  if (reply == QMessageBox::No)
  {
    return;
  }

  //    if( msgBox.clickedButton() == buttonPlaceholder )
  //    {
  //        for( auto& it: _mapped_plot_data.numeric )
  //        {
  //            it.second.clear();
  //        }
  //        for( auto& it: _mapped_plot_data.user_defined )
  //        {
  //            it.second.clear();
  //        }

  //        for(const auto& it: TabbedPlotWidget::instances())
  //        {
  //            PlotMatrix* matrix =  it.second->currentTab() ;
  //            matrix->maximumZoomOut(); // includes replot
  //        }
  //    }
  //    else
  {
    deleteAllData();
  }
}

void MainWindow::on_actionPreferences_triggered()
{
  PreferencesDialog dialog;
  dialog.exec();

  QSettings settings;
  QString theme = settings.value("Preferences::theme", _style_directory).toString();

  if (theme != _style_directory)
  {
    _style_directory = theme;
    emit stylesheetChanged(_style_directory);
  }
}

void MainWindow::on_playbackStep_valueChanged(double step)
{
  ui->timeSlider->setFocus();
  ui->timeSlider->setRealStepValue(step);
}
