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

//-------------------------------------------------

CurveListPanel::CurveListPanel(const CustomPlotMap &mapped_math_plots,
                               QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CurveListPanel),
//    _model(new QStandardItemModel(0, 2, this)),
//    _custom_model(new QStandardItemModel(0, 2, this)),
    //_tree_model(new TreeModel(_model)),
    _table_view( new CurveTableView( this) ),
    _custom_view( new CurveTableView( this) ),
    _tree_view( new CurveTreeView(this)),
    _custom_plots(mapped_math_plots)
{
    ui->setupUi(this);

    ui->verticalLayout->addWidget(_table_view, 1 );
    ui->verticalLayout->addWidget(_tree_view, 1 );
    ui->verticalLayoutCustom->addWidget( _custom_view, 1 );

    ui->widgetOptions->setVisible(false);

    ui->radioRegExp->setAutoExclusive(true);
    ui->radioContains->setAutoExclusive(true);

    QSettings settings;

    QString active_filter = settings.value("FilterableListWidget.searchFilter").toString();
    if( active_filter == "radioRegExp"){

        ui->radioRegExp->setChecked(true);
    }
    else if( active_filter == "radioContains"){

        ui->radioContains->setChecked(true);
    }

    int point_size = settings.value("FilterableListWidget/table_point_size", 9).toInt();
    changeFontSize(point_size);

    ui->splitter->setStretchFactor(0,5);
    ui->splitter->setStretchFactor(1,1);

    connect(  _custom_view->selectionModel(), &QItemSelectionModel::selectionChanged,
              this, &CurveListPanel::onCustomSelectionChanged );

    connect( _custom_view, &QAbstractItemView::pressed,
             _table_view,  & QAbstractItemView::clearSelection );

    connect( _table_view, &QAbstractItemView::pressed,
             _custom_view, & QAbstractItemView::clearSelection );

    connect( _table_view, &QAbstractItemView::pressed,
            _custom_view, & QAbstractItemView::clearSelection );

    connect( _table_view->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &CurveListPanel::refreshValues );

    connect( _custom_view->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &CurveListPanel::refreshValues );

    connect( _tree_view->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &CurveListPanel::refreshValues );

    connect( _tree_view, &QTreeWidget::itemExpanded,
            this, &CurveListPanel::refreshValues );

    bool tree = settings.value("FilterableListWidget/isTreeView", false).toBool();

    ui->radioFlat->setChecked( !tree );
    ui->radioTree->setChecked( tree );

    _tree_view->setHidden( !tree );
    _table_view->setHidden( tree );
}

CurveListPanel::~CurveListPanel()
{
    delete ui;
}

void CurveListPanel::clear()
{
    _table_view->setRowCount(0);
    _custom_view->setRowCount(0);
    _tree_view->clear();
    _numeric_data = nullptr;
    ui->labelNumberDisplayed->setText( "0 of 0");
}

void CurveListPanel::addCurve(const QString &item_name)
{
    _table_view->addItem(item_name);
    _tree_view->addItem(item_name);
}

void CurveListPanel::addCustom(const QString &item_name)
{
    _custom_view->addItem(item_name);
}

void CurveListPanel::refreshColumns()
{
    _table_view->refreshColumns();
    _tree_view->refreshColumns();
    _custom_view->refreshColumns();

    updateFilter();
}

void CurveListPanel::updateFilter()
{
    on_lineEdit_textChanged( ui->lineEdit->text() );
}

void CurveListPanel::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Delete){
        removeSelectedCurves();
    }
}

void CurveListPanel::changeFontSize(int point_size)
{
    _table_view->setFontSize(point_size);
    _custom_view->setFontSize(point_size);
    _tree_view->setFontSize(point_size);

    QSettings settings;
    settings.setValue("FilterableListWidget/table_point_size", point_size);
}

bool CurveListPanel::is2ndColumnHidden() const
{
    return ui->checkBoxHideSecondColumn->isChecked();
}

void CurveListPanel::update2ndColumnValues(double tracker_time,
                                           std::unordered_map<std::string, PlotData> *numeric_data)
{
    _tracker_time = tracker_time;
    _numeric_data = numeric_data;

    refreshValues();
}

void CurveListPanel::refreshValues()
{
    if( is2ndColumnHidden() || !_numeric_data ){
        return;
    }

    auto FormattedNumber = [](double value)
    {
        QString num_text = QString::number( value, 'f', 3);
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
        return num_text + " ";
    };

    auto GetValue = [&](const std::string& name) -> nonstd::optional<double>
    {
        auto it = _numeric_data->find(name);
        if( it !=  _numeric_data->end())
        {
            auto& data = it->second;

            if( _tracker_time < std::numeric_limits<double>::max())
            {
                auto value = data.getYfromX( _tracker_time );
                if(value){
                    return value;
                }
            }
            else if( data.size() > 0)
            {
                return data.back().y;
            }
        }
        return {};
    };

    //------------------------------------
    for(CurveTableView* table: { _table_view, _custom_view } )
    {
        const int vertical_height = table->visibleRegion().boundingRect().height();

        for (int row = 0; row < table->rowCount(); row++)
        {
            int vertical_pos = table->rowViewportPosition(row);
            if( vertical_pos < 0 || table->isRowHidden(row) ){ continue; }
            if( vertical_pos > vertical_height){ break; }

            const std::string& name = table->item(row,0)->text().toStdString();
            auto val = GetValue(name);
            if (val)
            {
                table->item(row, 1)->setText(FormattedNumber( val.value() ));
            }
        }
    }
    //------------------------------------
    {
        const int vertical_height = _tree_view->visibleRegion().boundingRect().height();

        auto DisplayValue = [&](QTreeWidgetItem* cell)
        {
            QString curve_name = cell->data( 0, Qt::UserRole ).toString();

            if( !curve_name.isEmpty() )
            {
                auto rect = cell->treeWidget()->visualItemRect(cell);

                if( rect.bottom() < 0 || cell->isHidden() ){ return; }
                if( rect.top() > vertical_height){ return; }

                auto val = GetValue( curve_name.toStdString() );
                if (val)
                {
                    cell->setText(1, FormattedNumber( val.value() ));
                }
            }
        };

        _tree_view->treeVisitor( DisplayValue );
    }
}

void CurveListPanel::on_radioContains_toggled(bool checked)
{
    if(checked) {
        updateFilter();
        ui->lineEdit->setCompleter( nullptr );
        QSettings settings;
        settings.setValue("FilterableListWidget.searchFilter", "radioContains");
    }
}

void CurveListPanel::on_radioRegExp_toggled(bool checked)
{
    if(checked) {
        updateFilter();
        ui->lineEdit->setCompleter( nullptr );
        QSettings settings;
        settings.setValue("FilterableListWidget.searchFilter", "radioRegExp");
    }
}

void CurveListPanel::on_checkBoxCaseSensitive_toggled(bool )
{
    updateFilter();
}

void CurveListPanel::on_lineEdit_textChanged(const QString &search_string)
{
    bool updated = false;

    CurvesView* active_view = ui->radioFlat->isChecked() ? (CurvesView*)_table_view : (CurvesView*)_tree_view;

    if (ui->radioRegExp->isChecked())
    {
        updated = active_view->applyVisibilityFilter(CurvesView::REGEX,
                                                     search_string);
    }
    else if (ui->radioContains->isChecked())
    {
        updated = active_view->applyVisibilityFilter(CurvesView::CONTAINS,
                                                     search_string);
    }

    auto h_c = active_view->hiddenItemsCount();
    int item_count = h_c.second;
    int visible_count = item_count - h_c.first;

    ui->labelNumberDisplayed->setText( QString::number( visible_count ) + QString(" of ") +
                                       QString::number( item_count ) );
    if(updated){
        emit hiddenItemsChanged();
    }
}

void CurveListPanel::on_pushButtonSettings_toggled(bool checked)
{
    ui->widgetOptions->setVisible(checked);
}

void CurveListPanel::on_checkBoxHideSecondColumn_toggled(bool checked)
{
    _tree_view->hideValuesColumn(checked);
    _table_view->hideValuesColumn(checked);
    _custom_view->hideValuesColumn(checked);
    emit hiddenItemsChanged();
}

void CurveListPanel::removeSelectedCurves()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, tr("Warning"),
                                  tr("Do you really want to remove these data?\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No );

    if (reply == QMessageBox::Yes)
    {
        emit deleteCurves( _table_view->getSelectedNames() );
        emit deleteCurves( _tree_view->getSelectedNames() );
        emit deleteCurves( _custom_view->getSelectedNames() );
    }

    updateFilter();
}

void CurveListPanel::removeCurve(const std::string &name)
{
    QString curve_name = QString::fromStdString(name);
   _table_view->removeCurve(curve_name);
   _tree_view->removeCurve(curve_name);
   _custom_view->removeCurve(curve_name);
}

void CurveListPanel::on_buttonAddCustom_clicked()
{
    std::array<CurvesView*,3> views = { _table_view, _tree_view, _custom_view };

    std::string suggested_name;
    for(CurvesView* view: views )
    {
        auto curve_names = view->getSelectedNames();
        if( curve_names.size() > 0)
        {
             suggested_name = ( curve_names.front() );
             break;
        }
    }

    emit createMathPlot(suggested_name);
    on_lineEdit_textChanged( ui->lineEdit->text() );
}


void CurveListPanel::onCustomSelectionChanged(const QItemSelection&, const QItemSelection &)
{
    auto selected = _custom_view->getSelectedNames();

    bool enabled = (selected.size() == 1);
    ui->buttonEditCustom->setEnabled( enabled );
    ui->buttonEditCustom->setToolTip( enabled ? "Edit the selected custom timeserie" :
                                                "Select a single custom Timeserie to Edit it");
}

void CurveListPanel::on_buttonEditCustom_clicked()
{
    QTableWidgetItem* selected_item = nullptr;

    for (QModelIndex index : _custom_view->selectionModel()->selectedRows(0))
    {
        selected_item = _custom_view->item( index.row(), 0 );
        break;
    }
    if( !selected_item )
    {
        return;
    }
    editMathPlot( selected_item->text().toStdString() );
}

void CurveListPanel::clearSelections()
{
    _custom_view->clearSelection();
    _tree_view->clearSelection();
    _table_view->clearSelection();
}

void CurveListPanel::on_stylesheetChanged(QString style_dir)
{
    ui->pushButtonSettings->setIcon(QIcon(tr(":/%1/settings_cog.png").arg(style_dir)));
}

void CurveListPanel::on_radioTree_toggled(bool checked)
{
    _tree_view->setVisible(checked);
    if( checked )
    {
        refreshValues();
        QSettings settings;
        settings.setValue("FilterableListWidget/isTreeView", true);
    }
}

void CurveListPanel::on_radioFlat_toggled(bool checked)
{
    _table_view->setVisible(checked);
    if( checked )
    {
        refreshValues();
        QSettings settings;
        settings.setValue("FilterableListWidget/isTreeView", false);
    }
}
