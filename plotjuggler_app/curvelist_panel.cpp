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

CurveListPanel::CurveListPanel(PlotDataMapRef& mapped_plot_data,
                               const CustomPlotMap& mapped_math_plots,
                               QWidget* parent)
  : QWidget(parent)
  , ui(new Ui::CurveListPanel)
  , _plot_data( mapped_plot_data )
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
  ui->labelNumberDisplayed->setText("0 of 0");
}

void CurveListPanel::addCurve(const std::string &plot_name)
{

  QString group_name;

  auto FindInPlotData = [&](auto& plot_data, const std::string &plot_name)
  {
    auto it = plot_data.find( plot_name );
    if( it != plot_data.end() ){
      auto& plot = it->second;
      if( plot.group() ){
        group_name = QString::fromStdString( plot.group()->name() );
      }
      return true;
    }
    return false;
  };

  bool found =
      FindInPlotData( _plot_data.numeric, plot_name ) ||
      FindInPlotData( _plot_data.strings, plot_name );

  if( !found ) {
    return;
  }

  QString plot_id = QString::fromStdString( plot_name );
  _tree_view->addItem(group_name, getTreeName( plot_id ), plot_id );

  _column_width_dirty = true;
}

void CurveListPanel::addCustom(const QString& item_name)
{
  _custom_view->addItem({}, item_name, item_name);
  _column_width_dirty = true;
}

void CurveListPanel::updateColors()
{
  QColor default_color = _tree_view->palette().color( QPalette::Text );
  //------------------------------------------
  // Propagate change in color and style to the children of a group
  std::function<void(QTreeWidgetItem*,QColor,bool)> ChangeColorAndStyle;
  ChangeColorAndStyle = [&](QTreeWidgetItem* cell, QColor color, bool italic)
  {
    cell->setForeground(0, color);
    auto font = cell->font(0);
    font.setItalic( italic );
    cell->setFont(0, font);
    for (int c = 0; c < cell->childCount(); c++)
    {
        ChangeColorAndStyle(cell->child(c), color, italic);
    };
  };

  // set everything to default first
  for (int c = 0; c < _tree_view->invisibleRootItem()->childCount(); c++)
  {
    ChangeColorAndStyle(_tree_view->invisibleRootItem()->child(c),
                        default_color,
                        false);
  }
  //------------- Change groups first ---------------------

  auto ChangeGroupVisitor = [&](QTreeWidgetItem* cell)
  {
    if( cell->data(0, CustomRoles::IsGroupName).toBool() )
    {
      auto group_name = cell->data(0, CustomRoles::Name).toString();
      auto it = _plot_data.groups.find( group_name.toStdString() );
      if ( it != _plot_data.groups.end() )
      {
        QVariant color_var = it->second->attribute("TextColor");
        QColor text_color = color_var.isValid() ?
              color_var.value<QColor>() : default_color;

        QVariant style_var = it->second->attribute("Italic");
        bool italic =( style_var.isValid() && style_var.value<bool>() );

        ChangeColorAndStyle(cell, text_color, italic);

        // tooltip doesn't propagate
        QVariant tooltip = it->second->attribute("ToolTip");
        cell->setData(0, CustomRoles::ToolTip, tooltip );
      }
    }
  };

  _tree_view->treeVisitor(ChangeGroupVisitor);

  //------------- Change leaves ---------------------

  auto ChangeLeavesVisitor = [&](QTreeWidgetItem* cell)
  {
    if( cell->childCount() == 0 )
    {
      const std::string& curve_name = cell->data(0, CustomRoles::Name).toString().toStdString();

      QVariant text_color;

      auto GetTextColor = [&](auto& plot_data, const std::string& curve_name){
        auto it = plot_data.find(curve_name);
        if ( it != plot_data.end() )
        {
          QVariant color_var = it->second.attribute("TextColor");
          if( color_var.isValid() )
          {
            cell->setForeground(0, color_var.value<QColor>() );
          }

          QVariant tooltip_var = it->second.attribute("ToolTip");
          cell->setData(0, CustomRoles::ToolTip, tooltip_var );

          QVariant style_var = it->second.attribute("Italic");
          bool italic = ( style_var.isValid() && style_var.value<bool>() );
          if( italic )
          {
            QFont font = cell->font(0);
            font.setItalic(italic);
            cell->setFont(0, font);
          }
          return true;
        }
        return false;
      };

      bool valid = ( GetTextColor( _plot_data.numeric, curve_name ) ||
                     GetTextColor( _plot_data.strings, curve_name ));
    }
  };

  _tree_view->treeVisitor(ChangeLeavesVisitor);
}

void CurveListPanel::refreshColumns()
{
  _tree_view->refreshColumns();
  _custom_view->refreshColumns();
  _column_width_dirty = false;

  updateFilter();

  updateColors();
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

void CurveListPanel::update2ndColumnValues(double tracker_time)
{
  _tracker_time = tracker_time;
  refreshValues();
}

void CurveListPanel::refreshValues()
{
  auto default_foreground = _custom_view->palette().foreground();

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

  auto GetValue = [&](const std::string& name) -> QString {
    {
      auto it = _plot_data.numeric.find(name);
      if (it != _plot_data.numeric.end())
      {
        auto& plot_data = it->second;
        auto val = plot_data.getYfromX(_tracker_time);
        if( val ) {
          return FormattedNumber(val.value());
        }
      }
    }

    {
      auto it = _plot_data.strings.find(name);
      if (it != _plot_data.strings.end())
      {
        auto& plot_data = it->second;
        auto val = plot_data.getYfromX(_tracker_time);
        if( val ) {
          auto str_view = val.value();
          if( str_view.back() == '\0') {
            return QString::fromLocal8Bit( str_view.data(), str_view.size() - 1 );
          }
          else{
            return QString::fromLocal8Bit( str_view.data(), str_view.size() );
          }
        }
      }
    }
    return "-";
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

      if ( !is2ndColumnHidden() )
      {
        const std::string& name = table->item(row, 0)->text().toStdString();
        QString str_value = GetValue(name);
        table->item(row, 1)->setText( str_value );
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
      QString curve_name = cell->data(0, CustomRoles::Name).toString();

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

        if ( !is2ndColumnHidden() )
        {
          QString str_value = GetValue(curve_name.toStdString());
          cell->setText(1, str_value );
        }
      }
    };

    tree_view->setViewResizeEnabled(false);
    tree_view->treeVisitor(DisplayValue);
    // tree_view->setViewResizeEnabled(true);
  }
}

QString StringifyArray(QString str)
{
  static const QRegExp rx("(\\[\\d+\\])");
  int pos = 0;
  std::vector<std::pair<int,int>> index_positions;

  while ((pos = rx.indexIn(str, pos)) != -1) {
    QString array_index = rx.cap(1);

    std::pair<int,int> index = {pos+1, array_index.size()-2};
    index_positions.push_back(index);
    pos += rx.matchedLength();
  }
  if( index_positions.empty() )
  {
    return str;
  }

  QStringList out_list;
  out_list.push_back(str);

  for(int i = index_positions.size()-1; i >= 0; i--)
  {
    std::pair<int,int> index = index_positions[i];
    str.remove( index.first, index.second );
    out_list.push_front(str);
  }

  return out_list.join("/");
}

QString CurveListPanel::getTreeName(QString name)
{
  auto parts = name.split('/', QString::SplitBehavior::SkipEmptyParts);

  QString out;
  for(int i=0; i < parts.size(); i++)
  {
    out += StringifyArray(parts[i]);
    if( i+1 < parts.size() )
    {
      out += "/";
    }
  }
  return out;
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
  ui->pushButtonTrash->setIcon(LoadSvgIcon(":/resources/svg/trash.svg", theme));
}

void CurveListPanel::on_checkBoxShowValues_toggled(bool show)
{
  _tree_view->hideValuesColumn(!show);
  _custom_view->hideValuesColumn(!show);
  emit hiddenItemsChanged();
}

void CurveListPanel::on_pushButtonTrash_clicked(bool )
{
  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Warning. Can't be undone.");
  msgBox.setText(tr("Delete data:\n\n"
                    "[Delete All]: remove timeseries and plots.\n"
                    "[Delete Points]: reset data points, but keep plots and timeseries.\n"
                    ));
  QPushButton* buttonAll = msgBox.addButton(tr("Delete All"), QMessageBox::NoRole);
  QPushButton* buttonPoints = msgBox.addButton(tr("Delete Points"), QMessageBox::NoRole);
  msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == buttonAll)
  {
    requestDeleteAll(1);
  }
  else if (msgBox.clickedButton() == buttonPoints) {
    requestDeleteAll(2);
  }
}
