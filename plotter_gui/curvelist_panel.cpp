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
}

CurveListPanel::~CurveListPanel()
{
    delete ui;
}

void CurveListPanel::clear()
{
   getTableModel()->setRowCount(0);
    //_tree_model->clear();
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


int CurveListPanel::findRowByName(const std::string &text) const
{
    auto item_list = getTableModel()->findItems( QString::fromStdString( text ), Qt::MatchExactly);
    if( item_list.isEmpty())
    {
        return -1;
    }
    if( item_list.count()>1)
    {
        qDebug() << "FilterableListWidget constins multiple rows with the same name";
        return -1;
    }
    return item_list.front()->row();
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

QTableView *CurveListPanel::getTableView() const
{
    return dynamic_cast<QTableView*>(_table_view);
}

QTableView *CurveListPanel::getCustomView() const
{
    return dynamic_cast<QTableView*>(_custom_view);
}

void CurveListPanel::changeFontSize(int point_size)
{
    _table_view->setFontSize(point_size);
    _custom_view->setFontSize(point_size);
    //_tree_view->setFontSize(point_size);

    QSettings settings;
    settings.setValue("FilterableListWidget/table_point_size", point_size);
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

    std::array<CurvesView*,2> views = {_table_view, _tree_view};

    for(auto& view: views)
    {
        if( ui->radioRegExp->isChecked())
        {
            updated = view->applyVisibilityFilter( CurvesView::REGEX, search_string );
        }
        else if( ui->radioContains->isChecked())
        {
            updated = view->applyVisibilityFilter( CurvesView::CONTAINS, search_string );
        }
    }

    auto h_c = _table_view->hiddenItemsCount();
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
    _table_view->hideValuesColumn(checked);
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
        emit deleteCurves( _table_view->getNonHiddenSelectedRows() );
    }

    updateFilter();
}

void CurveListPanel::removeRow(int row)
{
    getTableModel()->removeRow(row);
}

void CurveListPanel::on_buttonAddCustom_clicked()
{
    //TODO: may be the selection is in cutom curves
    auto curve_names = _table_view->getNonHiddenSelectedRows();

    if( curve_names.empty() )
    {
        emit createMathPlot("");
    }
    else
    {
        createMathPlot( curve_names.front() );
    }
    on_lineEdit_textChanged( ui->lineEdit->text() );
}


void CurveListPanel::onCustomSelectionChanged(const QItemSelection&, const QItemSelection &)
{
    auto selected = _custom_view->getNonHiddenSelectedRows();

    bool enabled = (selected.size() == 1);
    ui->buttonEditCustom->setEnabled( enabled );
    ui->buttonEditCustom->setToolTip( enabled ? "Edit the selected custom timeserie" :
                                                "Select a single custom Timeserie to Edit it");
}

void CurveListPanel::on_buttonEditCustom_clicked()
{
    QStandardItem* selected_item = nullptr;
    auto view = _custom_view;

    for (QModelIndex index : view->selectionModel()->selectedRows(0))
    {
        selected_item = getTableModel()->item( index.row(), 0 );
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
