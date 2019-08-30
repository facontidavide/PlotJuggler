#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <set>
#include <deque>
#include <functional>

#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QMainWindow>
#include <QSignalMapper>
#include <QShortcut>

#include "plotwidget.h"
#include "plotmatrix.h"
#include "curvelist_panel.h"
#include "tabbedplotwidget.h"
#include "subwindow.h"
#include "realslider.h"
#include "utils.h"
#include "PlotJuggler/dataloader_base.h"
#include "PlotJuggler/statepublisher_base.h"
#include "PlotJuggler/datastreamer_base.h"
#include "transforms/custom_function.h"

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QCommandLineParser& commandline_parser, QWidget *parent = nullptr);

    ~MainWindow();

    bool loadLayoutFromFile(QString filename, const bool& fromCommandline);
    bool loadDataFromFiles(QStringList filenames );
    bool loadDataFromFile(const FileLoadInfo &info);
    QString styleDirectory() const;

    bool yesNoMsgBox(const QString& label, const QString& msg);

public slots:

    void resizeEvent(QResizeEvent *) ;

    // Undo - Redo
    void onUndoableChange();
    void onUndoInvoked();
    void onRedoInvoked();

    // Actions in UI
    void on_streamingToggled();
    void on_pushButtonStreaming_toggled(bool streaming);
    void on_streamingSpinBox_valueChanged(int value);

    void on_splitterMoved(int, int);

    void onTrackerTimeUpdated(double absolute_time , bool do_replot);
    void onTrackerMovedFromWidget(QPointF pos );
    void onTimeSlider_valueChanged(double abs_time);

    void onPlotAdded(PlotWidget* plot);
    void onPlotMatrixAdded(PlotMatrix* matrix);

    void on_tabbedAreaDestroyed(QObject*object);

    void onFloatingWindowDestroyed(QObject*object);

    void onCreateFloatingWindow(PlotMatrix* first_tab = nullptr);

    void onSwapPlots(PlotWidget* source, PlotWidget* destination);

    void updateDataAndReplot(bool replot_hidden_tabs);

    void onUpdateLeftTableValues();

    void onDeleteMultipleCurves(const std::vector<std::string> &curve_names);

    void on_addMathPlot(const std::string &linked_name);
    void on_editMathPlot(const std::string &plot_name);
    void on_refreshMathPlot(const std::string &plot_name);

    void onPlaybackLoop();

private:

    Ui::MainWindow *ui;

    TabbedPlotWidget *_main_tabbed_widget;

    QShortcut _undo_shortcut;
    QShortcut _redo_shortcut;
    QShortcut _fullscreen_shortcut;
    QShortcut _streaming_shortcut;
    QShortcut _playback_shotcut;

    bool _minimized;

    CurveListPanel* _curvelist_widget;

    PlotDataMapRef  _mapped_plot_data;
    CustomPlotMap _custom_plots;

    std::map<QString,DataLoader*>      _data_loader;
    std::map<QString,StatePublisher*>  _state_publisher;
    std::map<QString,DataStreamer*>    _data_streamer;
    DataStreamer* _current_streamer;

    std::deque<QDomDocument> _undo_states;
    std::deque<QDomDocument> _redo_states;
    QElapsedTimer _undo_timer;
    bool _disable_undo_logging;

    bool _test_option;

    bool _autostart_publishers;

    bool _from_commandline;

    double _tracker_time;
    bool _track_realtime;

    std::vector<FileLoadInfo> _loaded_datafiles;
    CurveTracker::Parameter _tracker_param;

    std::map<CurveTracker::Parameter, QIcon> _tracker_button_icons;

    MonitoredValue _time_offset;

    QString _style_directory;

    QTimer *_replot_timer;
    QTimer *_publish_timer;

    QDateTime _prev_publish_time;

    void initializeActions();
    void initializePlugins(QString subdir_name);

    void forEachWidget(std::function<void(PlotWidget*, PlotMatrix*, int, int)> op);
    void forEachWidget(std::function<void(PlotWidget*)> op);

    void rearrangeGridLayout();

    QDomDocument xmlSaveState() const;
    bool xmlLoadState(QDomDocument state_document);

    void checkAllCurvesFromLayout(const QDomElement& root);

    void createTabbedDialog(QString suggest_win_name, PlotMatrix *first_tab);

    void importPlotDataMap(PlotDataMapRef &new_data, bool remove_old);

    bool isStreamingActive() const ;

    void closeEvent(QCloseEvent *event);

    void loadPluginState(const QDomElement &root);
    QDomElement savePluginState(QDomDocument &doc);

    std::tuple<double,double,int> calculateVisibleRangeX();

    void addOrEditMathPlot(const std::string &name, bool edit);

    void deleteAllData();

    void updateRecentDataMenu(QStringList new_filenames);
    void updateRecentLayoutMenu(QStringList new_filenames);

    void updatedDisplayTime();

    void updateTimeSlider();
    void updateTimeOffset();

    void buildDummyData();

signals:
    void requestRemoveCurveByName(const std::string& name);
    void activateStreamingMode( bool active);
    void activateTracker(bool active);
    void stylesheetChanged(QString);

public slots:
    void on_actionLoadData_triggered();
    void on_actionLoadLayout_triggered();
    void on_actionSaveLayout_triggered();
    void on_actionLoadDummyData_triggered();

    void on_actionFunctionEditor_triggered();
    void on_actionClearRecentData_triggered();
    void on_actionClearRecentLayout_triggered();

    void on_actionDeleteAllData_triggered();
    void on_actionClearBuffer_triggered();

    void on_actionFullscreen_triggered();

    void on_actionReportBug_triggered();
    void on_actionCheatsheet_triggered();
    void on_actionSupportPlotJuggler_triggered();
    void on_actionSaveAllPlotTabs_triggered();

    void on_actionStopStreaming_triggered();
    void on_actionAbout_triggered();
    void on_actionExit_triggered();

    void on_pushButtonOptions_toggled(bool checked);
    void on_pushButtonActivateGrid_toggled(bool checked);
    void on_pushButtonRatio_toggled(bool checked);
    void on_pushButtonPlay_toggled(bool checked);
    void on_pushButtonUseDateTime_toggled(bool checked);
    void on_pushButtonTimeTracker_pressed();
    void on_pushButtonRemoveTimeOffset_toggled(bool checked);

    void on_actionStartStreaming(QString streamer_name, const bool& fromCommandline);

private slots:
    void on_stylesheetChanged(QString style_name);
    void on_actionPreferences_triggered();
};



#endif // MAINWINDOW_H
