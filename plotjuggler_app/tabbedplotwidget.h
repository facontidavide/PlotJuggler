#ifndef TABBEDPLOTWIDGET_H
#define TABBEDPLOTWIDGET_H

#include <QWidget>
#include <QMainWindow>
#include <QTableWidget>
#include <QDomDocument>
#include "plot_docker.h"

class TabbedPlotWidget : public QWidget
{
  Q_OBJECT

public:
  typedef struct
  {
  } MainWindowArea;

  explicit TabbedPlotWidget(QString name,
                            QMainWindow* main_window,
                            PlotDataMapRef& mapped_data,
                            QMainWindow* parent);

  PlotDocker* currentTab();

  QTabWidget* tabWidget();

  const QTabWidget* tabWidget() const;

  PlotDocker* addTab(QString name);

  QDomElement xmlSaveState(QDomDocument& doc) const;

  bool xmlLoadState(QDomElement& tabbed_area);

  ~TabbedPlotWidget() override;

  QString name() const
  {
    return _name;
  }

  static const std::map<QString, TabbedPlotWidget*>& instances();

  static TabbedPlotWidget* instance(const QString& key);

  void setControlsVisible(bool visible);

public slots:

  void setStreamingMode(bool streaming_mode);

  //static void saveTabImage(QString fileName, PlotDocker* matrix);

  void on_stylesheetChanged(QString style_dir);

private slots:

  void on_renameCurrentTab();

  //void on_savePlotsToFile();

  void on_addTabButton_pressed();

  void on_tabWidget_currentChanged(int index);

  void on_tabWidget_tabCloseRequested(int index);

  void on_buttonLinkHorizontalScale_toggled(bool checked);

  void on_requestTabMovement(const QString& destination_name);

  void on_moveTabIntoNewWindow();

  // TODO void onMoveWidgetIntoNewTab(QString plot_name);

  void paintEvent(QPaintEvent* event) override;

private:

  QTabWidget* _tabWidget;

  QPushButton* _buttonHorizontalLink;
  QPushButton* _buttonLegend;
  QPushButton* _buttonAddTab;

  // TODO QAction* _action_savePlots;

 // QMenu* _tab_menu;

  const QString _name;

  QMainWindow* _main_window;

  PlotDataMapRef& _mapped_data;

  bool _horizontal_link;

  QString _parent_type;

  virtual void closeEvent(QCloseEvent* event) override;

 // void printPlotsNames();

protected:
  virtual bool eventFilter(QObject* obj, QEvent* event) override;

  static std::map<QString, TabbedPlotWidget*> _instances;

signals:
  void created();
  void undoableChange();
  void tabAdded(PlotDocker*);
  void sendTabToNewWindow(PlotDocker*);
};

#endif  // TABBEDPLOTWIDGET_H
