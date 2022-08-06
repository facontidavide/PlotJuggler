/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <QMenu>
#include <QSignalMapper>
#include <QAction>
#include <QTabBar>
#include <QSvgGenerator>
#include <QInputDialog>
#include <QMouseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QPainter>
#include <QTabWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include "qwt_plot_renderer.h"
#include "mainwindow.h"
#include "tabbedplotwidget.h"
#include "tab_widget.h"
#include "PlotJuggler/svg_util.h"

std::map<QString, TabbedPlotWidget*> TabbedPlotWidget::_instances;

TabbedPlotWidget::TabbedPlotWidget(QString name, QMainWindow* mainwindow,
                                   PlotDataMapRef& mapped_data, QMainWindow* parent)
  : QWidget(parent), _mapped_data(mapped_data), _name(name), _main_window(mainwindow)
{
  MainWindow* main_window = dynamic_cast<MainWindow*>(_main_window);

  setContentsMargins(0, 0, 0, 0);

  if (main_window == parent)
  {
    _parent_type = "main_window";
  }
  else
  {
    _parent_type = "floating_window";
  }

  if (TabbedPlotWidget::_instances.count(_name) > 0)
  {
    throw std::runtime_error("This is not supposed to happen");
  }
  // register this instance
  _instances[_name] = this;

  _horizontal_link = true;

  QHBoxLayout* main_layout = new QHBoxLayout(this);
  main_layout->setMargin(0);

  _tabWidget = new QTabWidget(this);
  _tabWidget->setTabsClosable(true);
  _tabWidget->setMovable(true);

  connect(_tabWidget->tabBar(), &QTabBar::tabBarDoubleClicked, this,
          &TabbedPlotWidget::on_renameCurrentTab);

  main_layout->addWidget(_tabWidget);

  connect(_tabWidget, &QTabWidget::currentChanged, this,
          &TabbedPlotWidget::on_tabWidget_currentChanged);

  tabWidget()->tabBar()->installEventFilter(this);

  // TODO _action_savePlots = new QAction(tr("&Save plots to file"), this);
  // TODO connect(_action_savePlots, &QAction::triggered, this,
  // &TabbedPlotWidget::on_savePlotsToFile);

  //  _tab_menu = new QMenu(this);
  //  _tab_menu->addSeparator();
  //  //_tab_menu->addAction(_action_savePlots);
  //  _tab_menu->addSeparator();

  connect(this, &TabbedPlotWidget::destroyed, main_window,
          &MainWindow::on_tabbedAreaDestroyed);
  connect(this, &TabbedPlotWidget::tabAdded, main_window, &MainWindow::onPlotTabAdded);
  connect(this, &TabbedPlotWidget::undoableChange, main_window,
          &MainWindow::onUndoableChange);

  // TODO connect(_tabWidget, &TabWidget::movingPlotWidgetToTab, this,
  // &TabbedPlotWidget::onMoveWidgetIntoNewTab);

  this->addTab({});

  _buttonAddTab = new QPushButton("", this);
  _buttonAddTab->setFlat(true);
  _buttonAddTab->setFixedSize(QSize(32, 32));
  _buttonAddTab->setFocusPolicy(Qt::NoFocus);

  connect(_buttonAddTab, &QPushButton::pressed, this,
          &TabbedPlotWidget::on_addTabButton_pressed);
}

void TabbedPlotWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  auto size = tabWidget()->tabBar()->size();
  _buttonAddTab->move(QPoint(size.width() + 5, 0));
}

PlotDocker* TabbedPlotWidget::currentTab()
{
  return static_cast<PlotDocker*>(tabWidget()->currentWidget());
}

QTabWidget* TabbedPlotWidget::tabWidget()
{
  return _tabWidget;
}

const QTabWidget* TabbedPlotWidget::tabWidget() const
{
  return _tabWidget;
}

PlotDocker* TabbedPlotWidget::addTab(QString tab_name)
{
  static int tab_suffix_count = 1;

  // this must be done before any PlotDocker is created
  ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasTabsMenuButton, false);
  ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
  ads::CDockManager::setConfigFlag(ads::CDockManager::AlwaysShowTabs, true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::EqualSplitOnInsertion, true);
  ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);

  if (tab_name.isEmpty())
  {
    tab_name = QString("tab%1").arg(tab_suffix_count++);
  }

  auto docker = new PlotDocker(tab_name, _mapped_data, this);
  connect(docker, &PlotDocker::undoableChange, this, &TabbedPlotWidget::undoableChange);

  tabWidget()->addTab(docker, tab_name);

  emit tabAdded(docker);
  // we need to send the signal for the very first widget
  emit docker->plotWidgetAdded(docker->plotAt(0));

  int index = tabWidget()->count() - 1;

  QWidget* button_widget = new QWidget();
  QHBoxLayout* layout = new QHBoxLayout(button_widget);
  layout->setSpacing(2);
  layout->setMargin(0);

  QPushButton* close_button = new QPushButton();

  QSettings settings;
  QString theme = settings.value("StyleSheet::theme", "light").toString();
  close_button->setIcon(LoadSvg(":/resources/svg/close-button.svg", theme));

  close_button->setFixedSize(QSize(16, 16));
  close_button->setFlat(true);
  connect(close_button, &QPushButton::pressed, this, [this]() {
    on_tabWidget_tabCloseRequested(tabWidget()->tabBar()->currentIndex());
  });

  layout->addWidget(close_button);
  tabWidget()->tabBar()->setTabButton(index, QTabBar::RightSide, button_widget);

  docker->setHorizontalLink(_horizontal_link);

  tabWidget()->setCurrentWidget(docker);

  return docker;
}

QDomElement TabbedPlotWidget::xmlSaveState(QDomDocument& doc) const
{
  QDomElement tabbed_area = doc.createElement("tabbed_widget");

  tabbed_area.setAttribute("name", _name);
  tabbed_area.setAttribute("parent", _parent_type);

  for (int i = 0; i < tabWidget()->count(); i++)
  {
    PlotDocker* widget = static_cast<PlotDocker*>(tabWidget()->widget(i));
    QDomElement element = widget->xmlSaveState(doc);

    element.setAttribute("tab_name", tabWidget()->tabText(i));
    tabbed_area.appendChild(element);
  }

  QDomElement current_plotmatrix = doc.createElement("currentTabIndex");
  current_plotmatrix.setAttribute("index", tabWidget()->currentIndex());
  tabbed_area.appendChild(current_plotmatrix);

  return tabbed_area;
}

bool TabbedPlotWidget::xmlLoadState(QDomElement& tabbed_area)
{
  int prev_count = tabWidget()->count();

  for (auto docker_elem = tabbed_area.firstChildElement("Tab"); !docker_elem.isNull();
       docker_elem = docker_elem.nextSiblingElement("Tab"))
  {
    QString tab_name = docker_elem.attribute("tab_name");
    PlotDocker* docker = addTab(tab_name);

    bool success = docker->xmlLoadState(docker_elem);

    if (!success)
    {
      return false;
    }
  }

  // remove old ones
  for (int i = 0; i < prev_count; i++)
  {
    tabWidget()->widget(0)->deleteLater();
    tabWidget()->removeTab(0);
  }

  QDomElement current_tab = tabbed_area.firstChildElement("currentTabIndex");
  int current_index = current_tab.attribute("index").toInt();

  if (current_index >= 0 && current_index < tabWidget()->count())
  {
    tabWidget()->setCurrentIndex(current_index);
  }

  emit undoableChange();
  return true;
}

void TabbedPlotWidget::setStreamingMode(bool streaming_mode)
{
}

TabbedPlotWidget::~TabbedPlotWidget()
{
}

void TabbedPlotWidget::on_renameCurrentTab()
{
  int idx = tabWidget()->tabBar()->currentIndex();

  bool ok = true;
  QString newName =
      QInputDialog::getText(this, tr("Change the tab name"), tr("New name:"),
                            QLineEdit::Normal, tabWidget()->tabText(idx), &ok);
  if (ok)
  {
    tabWidget()->setTabText(idx, newName);
    currentTab()->setName(newName);
  }
}

void TabbedPlotWidget::on_stylesheetChanged(QString theme)
{
  _buttonAddTab->setIcon(LoadSvg(":/resources/svg/add_tab.svg", theme));
}

void TabbedPlotWidget::on_addTabButton_pressed()
{
  addTab(nullptr);
  emit undoableChange();
}

void TabbedPlotWidget::on_tabWidget_currentChanged(int index)
{
  if (tabWidget()->count() == 0)
  {
    if (_parent_type.compare("main_window") == 0)
    {
      addTab(nullptr);
    }
    else
    {
      this->parent()->deleteLater();
    }
  }

  PlotDocker* tab = dynamic_cast<PlotDocker*>(tabWidget()->widget(index));
  if (tab)
  {
    tab->replot();
  }
  for (int i = 0; i < tabWidget()->count(); i++)
  {
    auto button = _tabWidget->tabBar()->tabButton(i, QTabBar::RightSide);
    if (button)
    {
      button->setHidden(i != index);
    }
  }
}

void TabbedPlotWidget::on_tabWidget_tabCloseRequested(int index)
{
  PlotDocker* tab = dynamic_cast<PlotDocker*>(tabWidget()->widget(index));

  // first add then delete.
  // Otherwise currentPlotGrid might be empty
  if (tabWidget()->count() == 1)
  {
    on_addTabButton_pressed();
  }

  PlotDocker* docker = static_cast<PlotDocker*>(tabWidget()->widget(index));

  for (unsigned p = 0; p < docker->plotCount(); p++)
  {
    PlotWidget* plot = docker->plotAt(p);
    plot->removeAllCurves();
    plot->deleteLater();
  }
  docker->deleteLater();

  tabWidget()->removeTab(index);
  emit undoableChange();
}

void TabbedPlotWidget::on_buttonLinkHorizontalScale_toggled(bool checked)
{
  _horizontal_link = checked;

  for (int i = 0; i < tabWidget()->count(); i++)
  {
    PlotDocker* tab = static_cast<PlotDocker*>(tabWidget()->widget(i));
    tab->setHorizontalLink(_horizontal_link);
  }
}

void TabbedPlotWidget::on_requestTabMovement(const QString& destination_name)
{
  TabbedPlotWidget* destination_widget = TabbedPlotWidget::_instances[destination_name];

  PlotDocker* tab_to_move = currentTab();
  int index = tabWidget()->tabBar()->currentIndex();

  const QString& tab_name = this->tabWidget()->tabText(index);

  destination_widget->tabWidget()->addTab(tab_to_move, tab_name);
  emit undoableChange();
}

void TabbedPlotWidget::on_moveTabIntoNewWindow()
{
  emit sendTabToNewWindow(currentTab());
}

bool TabbedPlotWidget::eventFilter(QObject* obj, QEvent* event)
{
  QTabBar* tab_bar = tabWidget()->tabBar();

  if (obj == tab_bar)
  {
    if (event->type() == QEvent::MouseButtonPress)
    {
      QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

      int index = tab_bar->tabAt(mouse_event->pos());
      tab_bar->setCurrentIndex(index);

      if (mouse_event->button() == Qt::RightButton)
      {
        // QMenu* submenu = new QMenu("Move tab to...");
        // _tab_menu->addMenu(submenu);

        // QSignalMapper* signalMapper = new QSignalMapper(submenu);

        //-----------------------------------
        //        QAction* action_new_window = submenu->addAction("New Window");
        //        submenu->addSeparator();
        //        connect(action_new_window, &QAction::triggered, this,
        //        &TabbedPlotWidget::on_moveTabIntoNewWindow);

        //        //-----------------------------------
        //        for (auto& it : TabbedPlotWidget::_instances)
        //        {
        //          QString name = it.first;
        //          TabbedPlotWidget* tabbed_menu = it.second;
        //          if (tabbed_menu != this)
        //          {
        //            QAction* action = submenu->addAction(name);
        //            connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
        //            signalMapper->setMapping(action, name);
        //          }
        //        }

        //        connect(signalMapper, SIGNAL(mapped(QString)), this,
        //        SLOT(on_requestTabMovement(QString)));

        //        //-------------------------------
        ////        QIcon iconSave;
        ////        iconSave.addFile(tr(":/%1/save.png").arg(theme), QSize(26, 26));
        ////        _action_savePlots->setIcon(iconSave);

        ////        QIcon iconNewWin;
        ////        iconNewWin.addFile(tr(":/%1/stacks.png").arg(theme), QSize(16, 16));
        ////        action_new_window->setIcon(iconNewWin);

        //        _tab_menu->exec(mouse_event->globalPos());
        //        //-------------------------------
        //        submenu->deleteLater();
      }
    }
  }

  // Standard event processing
  return QObject::eventFilter(obj, event);
}

void TabbedPlotWidget::closeEvent(QCloseEvent* event)
{
  TabbedPlotWidget::_instances.erase(name());
}

const std::map<QString, TabbedPlotWidget*>& TabbedPlotWidget::instances()
{
  return TabbedPlotWidget::_instances;
}

TabbedPlotWidget* TabbedPlotWidget::instance(const QString& key)
{
  auto it = TabbedPlotWidget::_instances.find(key);
  if (it == TabbedPlotWidget::_instances.end())
  {
    return nullptr;
  }
  else
  {
    return it->second;
  }
}

void TabbedPlotWidget::setControlsVisible(bool visible)
{
  // ui->widgetControls->setVisible(visible);
}
