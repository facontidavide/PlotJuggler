#include "curvelist_panel.h"
#include "ui_curvelist_panel.h"
#include "PlotJuggler/alphanum.hpp"
#include <QDebug>
#include <QLayoutItem>
#include <QMenu>
#include <QSettings>
#include <QDrag>
#include <QMimeData>
#include <QHeaderView>
#include <QFontDatabase>
#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <QCompleter>
#include <QStandardItem>
#include <QWheelEvent>
#include <QItemSelectionModel>
#include <QScrollBar>
#include <QTreeWidget>

#include "PlotJuggler/svg_util.h"

//-------------------------------------------------

CurveListPanel::CurveListPanel(const CustomPlotMap& mapped_math_plots, QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::CurveListPanel)
  , _custom_view(new CurveTableView(this))
  , _tree_view(new CurveTreeView(this))
  , _custom_plots(mapped_math_plots)
  , _column_width_dirty (true)
{
  ui->setupUi(this);

  setFocusPolicy(Qt::ClickFocus);

  _tree_view->setObjectName("curveTreeView");
  _custom_view->setObjectName("curveCustomView");

  auto layout1 = new QHBoxLayout();
  ui->listPlaceholder1->setLayout(layout1);
  layout1->addWidget(_tree_view, 1);
  layout1->setMargin(0);

  auto layout2 = new QHBoxLayout();
  ui->listPlaceholder2->setLayout(layout2);
  layout2->addWidget(_custom_view, 1);
  layout2->setMargin(0);

  QSettings settings;

  int point_size = settings.value("FilterableListWidget/table_point_size", 9).toInt();
  changeFontSize(point_size);

  ui->splitter->setStretchFactor(0, 5);
  ui->splitter->setStretchFactor(1, 1);

  connect(_custom_view->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &CurveListPanel::onCustomSelectionChanged);

  connect(_custom_view->verticalScrollBar(), &QScrollBar::valueChanged, this, &CurveListPanel::refreshValues);

  connect(_tree_view->verticalScrollBar(), &QScrollBar::valueChanged, this, &CurveListPanel::refreshValues);

  connect(_tree_view, &QTreeWidget::itemExpanded, this, &CurveListPanel::refreshValues);
}

CurveListPanel::~CurveListPanel()
{
  delete ui;
}

void CurveListPanel::clear()
{
  _custom_view->clear();
  _tree_view->clear();
  _numeric_data = nullptr;
  ui->labelNumberDisplayed->setText("0 of 0");
}

void CurveListPanel::addCurve(const QString& item_name)
{
  _tree_view->addItem(item_name);
  _column_width_dirty = true;
}

void CurveListPanel::addCustom(const QString& item_name)
{
  _custom_view->addItem(item_name);
  _column_width_dirty = true;
}

void CurveListPanel::refreshColumns()
{
  _tree_view->refreshColumns();
  _custom_view->refreshColumns();
  _column_width_dirty = false;
  updateFilter();
}

void CurveListPanel::updateFilter()
{
  on_lineEditFilter_textChanged(ui->lineEditFilter->text());
}

void CurveListPanel::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete)
  {
    removeSelectedCurves();
  }
}

void CurveListPanel::changeFontSize(int point_size)
{
  _custom_view->setFontSize(point_size);
  _tree_view->setFontSize(point_size);

  QSettings settings;
  settings.setValue("FilterableListWidget/table_point_size", point_size);
}

bool CurveListPanel::is2ndColumnHidden() const
{
  // return ui->checkBoxHideSecondColumn->isChecked();
  return false;
}

void CurveListPanel::update2ndColumnValues(double tracker_time, std::unordered_map<std::string, PlotData>* numeric_data)
{
  _tracker_time = tracker_time;
  _numeric_data = numeric_data;

  refreshValues();
}

void CurveListPanel::refreshValues()
{
  if (is2ndColumnHidden() || !_numeric_data)
  {
    return;
  }

  auto FormattedNumber = [](double value) {
    QString num_text = QString::number(value, 'f', 3);
    if (num_text.contains('.'))
    {
      int idx = num_text.length() - 1;
      while (num_text[idx] == '0')
      {
        num_text[idx] = ' ';
        idx--;
      }
      if (num_text[idx] == '.')
        num_text[idx] = ' ';
    }
    return num_text + " ";
  };

  auto GetValue = [&](const std::string& name) -> nonstd::optional<double> {
    auto it = _numeric_data->find(name);
    if (it != _numeric_data->end())
    {
      auto& data = it->second;

      if (_tracker_time < std::numeric_limits<double>::max())
      {
        auto value = data.getYfromX(_tracker_time);
        if (value)
        {
          return value;
        }
      }
      else if (data.size() > 0)
      {
        return data.back().y;
      }
    }
    return {};
  };

  //------------------------------------
  for (CurveTableView* table : { _custom_view })
  {
    table->setViewResizeEnabled(false);
    const int vertical_height = table->visibleRegion().boundingRect().height();

    for (int row = 0; row < table->rowCount(); row++)
    {
      int vertical_pos = table->rowViewportPosition(row);
      if (vertical_pos < 0 || table->isRowHidden(row))
      {
        continue;
      }
      if (vertical_pos > vertical_height)
      {
        break;
      }

      const std::string& name = table->item(row, 0)->text().toStdString();
      auto val = GetValue(name);
      if (val)
      {
        table->item(row, 1)->setText(FormattedNumber(val.value()));
      }
    }
    if(_column_width_dirty)
    {
      _column_width_dirty = false;
      table->setViewResizeEnabled(true);
    }
  }
  //------------------------------------
  for (CurveTreeView* tree_view : {_tree_view})
  {
    const int vertical_height = tree_view->visibleRegion().boundingRect().height();

    auto DisplayValue = [&](QTreeWidgetItem* cell) {
      QString curve_name = cell->data(0, Qt::UserRole).toString();

      if (!curve_name.isEmpty())
      {
        auto rect = cell->treeWidget()->visualItemRect(cell);

        if (rect.bottom() < 0 || cell->isHidden())
        {
          return;
        }
        if (rect.top() > vertical_height)
        {
          return;
        }

        auto val = GetValue(curve_name.toStdString());
        if (val)
        {
          cell->setText(1, FormattedNumber(val.value()));
        }
      }
    };

    tree_view->setViewResizeEnabled(false);
    tree_view->treeVisitor(DisplayValue);
    // tree_view->setViewResizeEnabled(true);
  }
}

void CurveListPanel::on_lineEditFilter_textChanged(const QString& search_string)
{
  bool updated = false;

  CurvesView* active_view = (CurvesView*)_tree_view;

  updated = active_view->applyVisibilityFilter(search_string);

  auto h_c = active_view->hiddenItemsCount();
  int item_count = h_c.second;
  int visible_count = item_count - h_c.first;

  ui->labelNumberDisplayed->setText(QString::number(visible_count) + QString(" of ") + QString::number(item_count));
  if (updated)
  {
    emit hiddenItemsChanged();
  }
}

void CurveListPanel::removeSelectedCurves()
{
  QMessageBox::StandardButton reply;
  reply = QMessageBox::question(nullptr, tr("Warning"), tr("Do you really want to remove these data?\n"),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

  if (reply == QMessageBox::Yes)
  {
    emit deleteCurves(_tree_view->getSelectedNames());
    emit deleteCurves(_custom_view->getSelectedNames());
  }

  updateFilter();
}

void CurveListPanel::removeCurve(const std::string& name)
{
  QString curve_name = QString::fromStdString(name);
  _tree_view->removeCurve(curve_name);
  _custom_view->removeCurve(curve_name);
}

void CurveListPanel::on_buttonAddCustom_clicked()
{
  std::array<CurvesView*, 2> views = { _tree_view, _custom_view };

  std::string suggested_name;
  for (CurvesView* view : views)
  {
    auto curve_names = view->getSelectedNames();
    if (curve_names.size() > 0)
    {
      suggested_name = (curve_names.front());
      break;
    }
  }

  emit createMathPlot(suggested_name);
  on_lineEditFilter_textChanged(ui->lineEditFilter->text());
}

void CurveListPanel::onCustomSelectionChanged(const QItemSelection&, const QItemSelection&)
{
  auto selected = _custom_view->getSelectedNames(); 

  bool enabled = (selected.size() == 1);
  ui->buttonEditCustom->setEnabled(enabled);
  ui->buttonEditCustom->setToolTip(enabled ? "Edit the selected custom timeserie" :
                                             "Select a single custom Timeserie to Edit it");
}

void CurveListPanel::on_buttonEditCustom_clicked()
{
  auto selected = _custom_view->getSelectedNames();
  if (selected.size() == 1)
  {
    editMathPlot(selected.front());
  }
}

std::vector<std::string> CurveListPanel::getSelectedNames() const
{
    auto selected =  _tree_view->getSelectedNames();
    auto custom_select =  _custom_view->getSelectedNames();
    selected.insert( selected.end(), custom_select.begin(), custom_select.end() );
    return selected;
}

void CurveListPanel::clearSelections()
{
  _custom_view->clearSelection();
  _tree_view->clearSelection();
}

void CurveListPanel::on_stylesheetChanged(QString theme)
{
  _style_dir = theme;
  ui->buttonAddCustom->setIcon(LoadSvgIcon(":/resources/svg/add_tab.svg", theme));
  ui->buttonEditCustom->setIcon(LoadSvgIcon(":/resources/svg/pencil-edit.svg", theme));
}

void CurveListPanel::on_checkBoxShowValues_toggled(bool show)
{
  _tree_view->hideValuesColumn(!show);
  _custom_view->hideValuesColumn(!show);
  emit hiddenItemsChanged();
}
