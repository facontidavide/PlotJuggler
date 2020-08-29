#include <QMenu>
#include <QSignalMapper>
#include <QAction>
#include <QSvgGenerator>
#include <QInputDialog>
#include <QMouseEvent>
#include <QFileDialog>
#include <QApplication>
#include <QPainter>
#include "qwt_plot_renderer.h"
#include "mainwindow.h"
#include "tabbedplotwidget.h"
#include "tab_widget.h"
#include "ui_tabbedplotwidget.h"

std::map<QString, TabbedPlotWidget*> TabbedPlotWidget::_instances;

TabbedPlotWidget::TabbedPlotWidget(QString name, QMainWindow* mainwindow, PlotMatrix* first_tab,
                                   PlotDataMapRef& mapped_data, QMainWindow* parent)
  : QWidget(parent)
  , _mapped_data(mapped_data)
  , ui(new Ui::TabbedPlotWidget)
  , _name(name)
  , _main_window(mainwindow)
  , _labels_status(LabelStatus::RIGHT)
{
  MainWindow* main_window = dynamic_cast<MainWindow*>(_main_window);

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

  ui->setupUi(this);

  _horizontal_link = true;

  tabWidget()->tabBar()->installEventFilter(this);

  _action_renameTab = new QAction(tr("Rename tab"), this);
  connect(_action_renameTab, &QAction::triggered, this, &TabbedPlotWidget::on_renameCurrentTab);

  _action_savePlots = new QAction(tr("&Save plots to file"), this);
  connect(_action_savePlots, &QAction::triggered, this, &TabbedPlotWidget::on_savePlotsToFile);

  _tab_menu = new QMenu(this);
  _tab_menu->addAction(_action_renameTab);
  _tab_menu->addSeparator();
  _tab_menu->addAction(_action_savePlots);
  _tab_menu->addSeparator();

  connect(this, &TabbedPlotWidget::destroyed, main_window, &MainWindow::on_tabbedAreaDestroyed);
  connect(this, &TabbedPlotWidget::sendTabToNewWindow, main_window, &MainWindow::onCreateFloatingWindow);
  connect(this, &TabbedPlotWidget::matrixAdded, main_window, &MainWindow::onPlotMatrixAdded);
  connect(this, &TabbedPlotWidget::undoableChangeHappened, main_window, &MainWindow::onUndoableChange);

  connect(ui->tabWidget, &TabWidget::movingPlotWidgetToTab, this, &TabbedPlotWidget::onMoveWidgetIntoNewTab);

  this->addTab(first_tab);

  this->layout()->removeWidget(ui->widgetControls);
  ui->tabWidget->setCornerWidget(ui->widgetControls);
}

// void TabbedPlotWidget::setSiblingsList(const std::map<QString, TabbedPlotWidget *> &other_tabbed_widgets)
//{
//    _other_siblings = other_tabbed_widgets;
//}

PlotMatrix* TabbedPlotWidget::currentTab()
{
  return static_cast<PlotMatrix*>(tabWidget()->currentWidget());
}

QTabWidget* TabbedPlotWidget::tabWidget()
{
  return ui->tabWidget;
}

const QTabWidget* TabbedPlotWidget::tabWidget() const
{
  return ui->tabWidget;
}

void TabbedPlotWidget::addTab(PlotMatrix* tab)
{
  if (!tab)
  {
    tab = new PlotMatrix("plot", _mapped_data, this);
    tabWidget()->addTab(tab, QString("plot"));

    QApplication::processEvents();
    emit matrixAdded(tab);
    tab->addColumn();
  }
  else
  {
    tabWidget()->addTab(tab, tab->name());
  }

  tabWidget()->setCurrentWidget(tab);
  tab->setHorizontalLink(_horizontal_link);
}

QDomElement TabbedPlotWidget::xmlSaveState(QDomDocument& doc) const
{
  QDomElement tabbed_area = doc.createElement("tabbed_widget");

  tabbed_area.setAttribute("name", _name);
  tabbed_area.setAttribute("parent", _parent_type);

  for (int i = 0; i < tabWidget()->count(); i++)
  {
    PlotMatrix* widget = static_cast<PlotMatrix*>(tabWidget()->widget(i));
    QDomElement element = widget->xmlSaveState(doc);

    element.setAttribute("tab_name", tabWidget()->tabText(i));
    tabbed_area.appendChild(element);
  }

  QDomElement current_plotmatrix = doc.createElement("currentPlotMatrix");
  current_plotmatrix.setAttribute("index", tabWidget()->currentIndex());
  tabbed_area.appendChild(current_plotmatrix);

  return tabbed_area;
}

bool TabbedPlotWidget::xmlLoadState(QDomElement& tabbed_area)
{
  int num_tabs = tabWidget()->count();
  int index = 0;

  QDomElement plotmatrix_el;

  for (plotmatrix_el = tabbed_area.firstChildElement("plotmatrix"); !plotmatrix_el.isNull();
       plotmatrix_el = plotmatrix_el.nextSiblingElement("plotmatrix"))
  {
    // add if tabs are too few
    if (index == num_tabs)
    {
      this->addTab(NULL);
      num_tabs++;
    }

    PlotMatrix* plot_matrix = static_cast<PlotMatrix*>(tabWidget()->widget(index));
    // read tab name
    if (plotmatrix_el.hasAttribute("tab_name"))
    {
      QString tab_name = plotmatrix_el.attribute("tab_name");
      tabWidget()->setTabText(index, tab_name);
      plot_matrix->setName(tab_name);
    }

    bool success = plot_matrix->xmlLoadState(plotmatrix_el);

    if (!success)
    {
      return false;
    }

    index++;
  }

  // remove if tabs are too much
  while (num_tabs > index)
  {
    tabWidget()->removeTab(num_tabs - 1);
    num_tabs--;
  }

  QDomElement current_plotmatrix = tabbed_area.firstChildElement("currentPlotMatrix");
  int current_index = current_plotmatrix.attribute("index").toInt();

  if (current_index >= 0 && current_index < tabWidget()->count())
  {
    tabWidget()->setCurrentIndex(current_index);
  }
  return true;
}

void TabbedPlotWidget::setStreamingMode(bool streaming_mode)
{
  ui->buttonLinkHorizontalScale->setEnabled(!streaming_mode);
  ui->pushVerticalResize->setEnabled(!streaming_mode);
  ui->pushHorizontalResize->setEnabled(!streaming_mode);
}

TabbedPlotWidget::~TabbedPlotWidget()
{
  delete ui;
}

void TabbedPlotWidget::on_renameCurrentTab()
{
  int idx = tabWidget()->tabBar()->currentIndex();

  bool ok = true;
  QString newName = QInputDialog::getText(this, tr("Change Name of the selected tab"), tr("Insert New Tab Name"),
                                          QLineEdit::Normal, tabWidget()->tabText(idx), &ok);

  if (ok)
  {
    tabWidget()->setTabText(idx, newName);
    currentTab()->setName(newName);
  }
}

void TabbedPlotWidget::on_savePlotsToFile()
{
  int idx = tabWidget()->tabBar()->currentIndex();
  PlotMatrix* matrix = static_cast<PlotMatrix*>(tabWidget()->widget(idx));

  QFileDialog saveDialog(this);
  saveDialog.setAcceptMode(QFileDialog::AcceptSave);
  saveDialog.selectFile(currentTab()->name());

  QStringList filters;
  filters << "png (*.png)"
          << "jpg (*.jpg *.jpeg)"
          << "svg (*.svg)";

  saveDialog.setNameFilters(filters);
  saveDialog.exec();

  if (saveDialog.result() == QDialog::Accepted && !saveDialog.selectedFiles().empty())
  {
    QString fileName = saveDialog.selectedFiles().first();

    QFileInfo fileinfo(fileName);
    if (fileinfo.suffix().isEmpty())
    {
      auto filter = saveDialog.selectedNameFilter();
      if (filter == filters[0])
      {
        fileName.append(".png");
      }
      else if (filter == filters[1])
      {
        fileName.append(".jpg");
      }
      else if (filter == filters[2])
      {
        fileName.append(".svg");
      }
    }
    saveTabImage(fileName, matrix);
  }
}

void TabbedPlotWidget::saveTabImage(QString fileName, PlotMatrix* matrix)
{
  bool is_svg = (QFileInfo(fileName).suffix().toLower() == "svg");

  QPixmap pixmap(1200, 900);
  QRect documentRect(0, 0, 1200, 900);

  QSvgGenerator generator;
  QPainter* painter = nullptr;

  if (is_svg)
  {
    generator.setFileName(fileName);
    generator.setResolution(80);
    generator.setViewBox(documentRect);
    painter = new QPainter(&generator);
  }
  else
  {
    painter = new QPainter(&pixmap);
  }

  if (fileName.isEmpty())
  {
    return;
  }

  QwtPlotRenderer rend;

  int delta_X = pixmap.width() / matrix->colsCount();
  int delta_Y = pixmap.height() / matrix->rowsCount();

  for (unsigned c = 0; c < matrix->colsCount(); c++)
  {
    for (unsigned r = 0; r < matrix->rowsCount(); r++)
    {
      PlotWidget* widget = matrix->plotAt(r, c);
      bool tracker_enabled = widget->isTrackerEnabled();
      if (tracker_enabled)
      {
        widget->enableTracker(false);
        widget->replot();
      }

      QRect rect(delta_X * c, delta_Y * r, delta_X, delta_Y);
      rend.render(widget, painter, rect);

      if (tracker_enabled)
      {
        widget->enableTracker(true);
        widget->replot();
      }
    }
  }
  painter->end();
  if (!is_svg)
  {
    pixmap.save(fileName);
  }
}

void TabbedPlotWidget::on_stylesheetChanged(QString style_dir)
{
  ui->pushButtonZoomMax->setIcon(QIcon(tr(":/%1/zoom_max.png").arg(style_dir)));
  ui->pushVerticalResize->setIcon(QIcon(tr(":/%1/zoom_vertical.png").arg(style_dir)));
  ui->pushHorizontalResize->setIcon(QIcon(tr(":/%1/zoom_horizontal.png").arg(style_dir)));
  ui->pushAddColumn->setIcon(QIcon(tr(":/%1/add_column.png").arg(style_dir)));
  ui->pushAddRow->setIcon(QIcon(tr(":/%1/add_row.png").arg(style_dir)));
  ui->addTabButton->setIcon(QIcon(tr(":/%1/add_tab.png").arg(style_dir)));
  ui->pushRemoveEmpty->setIcon(QIcon(tr(":/%1/clean_pane.png").arg(style_dir)));
  ui->pushButtonShowLabel->setIcon(QIcon(tr(":/%1/list.png").arg(style_dir)));
  ui->buttonLinkHorizontalScale->setIcon(QIcon(tr(":/%1/link.png").arg(style_dir)));
}

void TabbedPlotWidget::on_pushAddRow_pressed()
{
  currentTab()->addRow();
  onLabelStatusChanged();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_pushAddColumn_pressed()
{
  currentTab()->addColumn();
  onLabelStatusChanged();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_pushVerticalResize_pressed()
{
  currentTab()->maximumZoomOutVertical();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_pushHorizontalResize_pressed()
{
  currentTab()->maximumZoomOutHorizontal();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_pushButtonZoomMax_pressed()
{
  currentTab()->maximumZoomOut();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::printPlotsNames()
{
  for (int t = 0; t < tabWidget()->count(); t++)
  {
    PlotMatrix* matrix = static_cast<PlotMatrix*>(tabWidget()->widget(t));
    for (unsigned row = 0; row < matrix->rowsCount(); row++)
    {
      for (unsigned col = 0; col < matrix->colsCount(); col++)
      {
        PlotWidget* plot = matrix->plotAt(row, col);
        qDebug() << plot->windowTitle() << " at " << row << "/" << col << "/" << t;
      }
    }
  }
  qDebug() << "----------";
}

void TabbedPlotWidget::onMoveWidgetIntoNewTab(QString plot_name)
{
  int src_row, src_col;
  PlotMatrix* src_matrix = nullptr;
  PlotWidget* source = nullptr;

  for (int t = 0; t < tabWidget()->count(); t++)
  {
    PlotMatrix* matrix = static_cast<PlotMatrix*>(tabWidget()->widget(t));

    for (unsigned row = 0; row < matrix->rowsCount(); row++)
    {
      for (unsigned col = 0; col < matrix->colsCount(); col++)
      {
        PlotWidget* plot = matrix->plotAt(row, col);
        if (plot->windowTitle() == plot_name)
        {
          src_matrix = matrix;
          src_row = row;
          src_col = col;
          source = plot;
          break;
        }
      }
    }
  }

  addTab();
  PlotMatrix* dst_matrix = currentTab();
  PlotWidget* destination = dst_matrix->plotAt(0, 0);

  src_matrix->gridLayout()->removeWidget(source);
  dst_matrix->gridLayout()->removeWidget(destination);

  src_matrix->gridLayout()->addWidget(destination, src_row, src_col);
  dst_matrix->gridLayout()->addWidget(source, 0, 0);
  source->changeBackgroundColor(Qt::white);
  destination->changeBackgroundColor(Qt::white);

  src_matrix->removeEmpty();
  src_matrix->updateLayout();
  dst_matrix->updateLayout();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_addTabButton_pressed()
{
  addTab(nullptr);
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_pushRemoveEmpty_pressed()
{
  currentTab()->removeEmpty();
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_tabWidget_currentChanged(int index)
{
  if (tabWidget()->count() == 0)
  {
    if (_parent_type.compare("main_window") == 0)
    {
      addTab(NULL);
    }
    else
    {
      this->parent()->deleteLater();
    }
  }

  PlotMatrix* tab = static_cast<PlotMatrix*>(tabWidget()->widget(index));
  if (tab)
  {
    tab->replot();
  }
}

void TabbedPlotWidget::on_tabWidget_tabCloseRequested(int index)
{
  PlotMatrix* tab = static_cast<PlotMatrix*>(tabWidget()->widget(index));

  bool close_confirmed = true;
  if (tab->plotCount() == 1)
  {
    if (tab->plotAt(0)->isEmpty())
    {
      close_confirmed = false;
    }
  }

  QMessageBox::StandardButton do_remove = QMessageBox::Yes;

  if (close_confirmed)
  {
    tabWidget()->setCurrentIndex(index);
    QApplication::processEvents();

    do_remove = QMessageBox::question(this, tr("Warning"), tr("Do you really want to destroy this tab?\n"),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  }
  if (do_remove == QMessageBox::Yes)
  {
    // first add then delete.
    // Otherwise currentPlotGrid might be empty
    if (tabWidget()->count() == 1)
    {
      on_addTabButton_pressed();
    }

    PlotMatrix* matrix = static_cast<PlotMatrix*>(tabWidget()->widget(index));

    for (unsigned p = 0; p < matrix->plotCount(); p++)
    {
      PlotWidget* plot = matrix->plotAt(p);
      plot->removeAllCurves();
      plot->deleteLater();
    }
    matrix->deleteLater();

    tabWidget()->removeTab(index);
    emit undoableChangeHappened();
  }
}

void TabbedPlotWidget::on_buttonLinkHorizontalScale_toggled(bool checked)
{
  _horizontal_link = checked;

  for (int i = 0; i < tabWidget()->count(); i++)
  {
    PlotMatrix* tab = static_cast<PlotMatrix*>(tabWidget()->widget(i));
    tab->setHorizontalLink(_horizontal_link);
  }
}

void TabbedPlotWidget::on_requestTabMovement(const QString& destination_name)
{
  TabbedPlotWidget* destination_widget = TabbedPlotWidget::_instances[destination_name];

  PlotMatrix* tab_to_move = currentTab();
  int index = tabWidget()->tabBar()->currentIndex();

  const QString& tab_name = this->tabWidget()->tabText(index);

  destination_widget->tabWidget()->addTab(tab_to_move, tab_name);
  emit undoableChangeHappened();
}

void TabbedPlotWidget::on_moveTabIntoNewWindow()
{
  emit sendTabToNewWindow(currentTab());
}

void TabbedPlotWidget::on_pushButtonShowLabel_pressed()
{
  switch (_labels_status)
  {
    case LabelStatus::LEFT:
      _labels_status = LabelStatus::HIDDEN;
      break;
    case LabelStatus::RIGHT:
      _labels_status = LabelStatus::LEFT;
      break;
    case LabelStatus::HIDDEN:
      _labels_status = LabelStatus::RIGHT;
      break;
  }
  onLabelStatusChanged();
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
        QMenu* submenu = new QMenu("Move tab to...");
        _tab_menu->addMenu(submenu);

        QSignalMapper* signalMapper = new QSignalMapper(submenu);

        //-----------------------------------
        QAction* action_new_window = submenu->addAction("New Window");
        submenu->addSeparator();
        connect(action_new_window, &QAction::triggered, this, &TabbedPlotWidget::on_moveTabIntoNewWindow);

        //-----------------------------------
        for (auto& it : TabbedPlotWidget::_instances)
        {
          QString name = it.first;
          TabbedPlotWidget* tabbed_menu = it.second;
          if (tabbed_menu != this)
          {
            QAction* action = submenu->addAction(name);
            connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
            signalMapper->setMapping(action, name);
          }
        }

        connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(on_requestTabMovement(QString)));

        //-------------------------------
        QString theme = static_cast<MainWindow*>(_main_window)->styleDirectory();
        QIcon iconSave;
        iconSave.addFile(tr(":/%1/save.png").arg(theme), QSize(26, 26));
        _action_savePlots->setIcon(iconSave);

        QIcon iconNewWin;
        iconNewWin.addFile(tr(":/%1/stacks.png").arg(theme), QSize(16, 16));
        action_new_window->setIcon(iconNewWin);

        _tab_menu->exec(mouse_event->globalPos());
        //-------------------------------
        submenu->deleteLater();
      }
    }
  }

  // Standard event processing
  return QObject::eventFilter(obj, event);
}

void TabbedPlotWidget::onLabelStatusChanged()
{
  for (int i = 0; i < tabWidget()->count(); i++)
  {
    PlotMatrix* matrix = static_cast<PlotMatrix*>(tabWidget()->widget(i));

    for (unsigned p = 0; p < matrix->plotCount(); p++)
    {
      PlotWidget* plot = matrix->plotAt(p);

      plot->activateLegend(_labels_status != LabelStatus::HIDDEN);
      if (_labels_status == LabelStatus::LEFT)
      {
        plot->setLegendAlignment(Qt::AlignLeft);
      }
      else if (_labels_status == LabelStatus::RIGHT)
      {
        plot->setLegendAlignment(Qt::AlignRight);
      }
      plot->replot();
    }
  }
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
  ui->widgetControls->setVisible(visible);
}
