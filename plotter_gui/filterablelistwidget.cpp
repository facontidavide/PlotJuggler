#include "filterablelistwidget.h"
#include "ui_filterablelistwidget.h"
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
#include <QItemSelectionModel>

class CustomSortedTableItem: public QStandardItem
{

public:
    CustomSortedTableItem(const QString& name): QStandardItem(name), str(name.toStdString()) {}

    bool operator< (const CustomSortedTableItem &other) const
    {
        return doj::alphanum_impl(this->str.c_str(),
                                  other.str.c_str()) < 0;
    }
private:
    std::string str;
};


//-------------------------------------------------

FilterableListWidget::FilterableListWidget(const std::unordered_map<std::string, MathPlotPtr>& mapped_math_plots, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterableListWidget),
    _completer( new TreeModelCompleter(this) ),
    _mapped_math_plots(mapped_math_plots)
{
    ui->setupUi(this);
    _table_view = ui->tableView;
    _table_view->viewport()->installEventFilter( this );

    _model = new QStandardItemModel(0, 2, this);
    _table_view->setModel( _model );
    _table_view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    _table_view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    _table_view->horizontalHeader()->resizeSection(1, 120);

    ui->widgetOptions->setVisible(false);

    ui->radioRegExp->setAutoExclusive(true);
    ui->radioContains->setAutoExclusive(true);
    ui->radioPrefix->setAutoExclusive(true);

    _completer->setCompletionMode( QCompleter::PopupCompletion );

    QSettings settings;

    QString active_filter = settings.value("FilterableListWidget.searchFilter").toString();
    if( active_filter == "radioRegExp"){

        ui->radioRegExp->setChecked(true);
    }
    else if( active_filter == "radioPrefix"){

        ui->radioPrefix->setChecked(true);
    }
    else if( active_filter == "radioContains"){

        ui->radioContains->setChecked(true);
    }

    _completer_need_update = ui->radioPrefix->isChecked();
    ui->lineEdit->setCompleter( _completer_need_update ? _completer : nullptr );
}

FilterableListWidget::~FilterableListWidget()
{
    delete ui;
}

int FilterableListWidget::rowCount() const
{
    return _model->rowCount();
}

void FilterableListWidget::clear()
{
    _model->setRowCount(0);
    _completer->clear();
    ui->labelNumberDisplayed->setText( "0 of 0");
}


void FilterableListWidget::addItem(const QString &item_name)
{
    auto item = new CustomSortedTableItem(item_name);
    const int row = rowCount();
    _model->setRowCount(row+1);
    _model->setItem(row, 0, item);

    auto val_cell = new QStandardItem("-");
    val_cell->setTextAlignment(Qt::AlignRight);
    val_cell->setFlags( Qt::NoItemFlags | Qt::ItemIsEnabled );
    val_cell->setFont(  QFontDatabase::systemFont(QFontDatabase::FixedFont) );

    _model->setItem(row, 1, val_cell );

    if( _completer_need_update )
    {
        _completer->addToCompletionTree(item_name);
    }
}

void FilterableListWidget::refreshColumns()
{
    _table_view->sortByColumn(0,Qt::AscendingOrder);
    updateFilter();
}


int FilterableListWidget::findRowByName(const std::string &text) const
{
    auto item_list = _model->findItems( QString::fromStdString( text ), Qt::MatchExactly);
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


void FilterableListWidget::updateFilter()
{
    on_lineEdit_textChanged( ui->lineEdit->text() );
}

void FilterableListWidget::keyPressEvent(QKeyEvent *event)
{
    if( event->key() == Qt::Key_Delete){
        removeSelectedCurves();
    }
}

bool FilterableListWidget::eventFilter(QObject *object, QEvent *event)
{
    QObject *obj = object;
    while ( obj != NULL )
    {
        if( obj == _table_view || obj == ui->lineEdit ) break;
        obj = obj->parent();
    }

    //Ignore obj different than tableView
    if(obj != _table_view)
    {
        return QWidget::eventFilter(object,event);
    }

    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);

        _dragging = false;
        _drag_start_pos = mouse_event->pos();

        if(mouse_event->button() == Qt::LeftButton )
        {
            _newX_modifier = false;
        }
        else if(mouse_event->button() == Qt::RightButton )
        {
            //_newX_modifier = true;

            QPoint mousepos = mouse_event->globalPos();
            QModelIndex index = ui->tableView->indexAt(ui->tableView->mapFromGlobal(mousepos));
	        if(index.isValid())
	        {

                QString plotName = ui->tableView->model()->data(ui->tableView->model()->index(index.row(), 0)).toString();
                qDebug() << "Editing equation : " << plotName;

	            QMenu menu(this);
                QAction *createPlotAction = menu.addAction("Create math plot");
                QAction *editPlotAction = menu.addAction("Edit math plot");
                QAction *refreshPlotAction = menu.addAction("Refresh math plot");
                menu.addSeparator();
                QAction *deleteAction = menu.addAction("Delete");

                if(_mapped_math_plots.count(plotName.toStdString()) == 0)
                {
                    editPlotAction->setVisible(false);
                    refreshPlotAction->setVisible(false);
                }

                QAction *userAction = menu.exec(mouse_event->globalPos());
                if(userAction == createPlotAction)
                {
                    emit createMathPlot(plotName.toStdString());
                }
                else if(userAction == editPlotAction)
                {
                    emit editMathPlot(plotName.toStdString());
                }
                else if(userAction == refreshPlotAction)
                {
                    emit refreshMathPlot(plotName.toStdString());
                }
                else if(userAction == deleteAction)
                {
                    askToRemoveCurves({plotName});
                }

	            return false;
	        }
        }
        else {
            return false;
        }
        return QWidget::eventFilter(object,event);
    }
    else if(event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
        double distance_from_click = (mouse_event->pos() - _drag_start_pos).manhattanLength();

        if ((mouse_event->buttons() == Qt::LeftButton ||
             mouse_event->buttons() == Qt::RightButton) &&
                distance_from_click >= QApplication::startDragDistance() &&
                _dragging == false)
        {
            _dragging = true;
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;

            QByteArray mdata;
            QDataStream stream(&mdata, QIODevice::WriteOnly);

            for(const auto& selected_index: getNonHiddenSelectedRows())
            {
                auto item = _model->item( selected_index.row(), 0 );
                stream << item->text();
            }

            if( !_newX_modifier )
            {
                mimeData->setData("curveslist/add_curve", mdata);
            }
            else
            {
                if(getNonHiddenSelectedRows().size() != 1)
                {
                    return false;
                }
                mimeData->setData("curveslist/new_X_axis", mdata);

                QPixmap cursor( QSize(160,30) );
                cursor.fill(Qt::transparent);

                QPainter painter;
                painter.begin( &cursor);
                painter.setPen(QColor(22, 22, 22));

                QString text("set as new X axis");
                painter.setFont( QFont("Arial", 14 ) );

                painter.setBackground(Qt::transparent);
                painter.drawText( QRect(0, 0, 160, 30), Qt::AlignHCenter | Qt::AlignVCenter, text );
                painter.end();

                drag->setDragCursor(cursor, Qt::MoveAction);
            }

            drag->setMimeData(mimeData);
            drag->exec(Qt::CopyAction | Qt::MoveAction);
        }
        return true;
    }

    return QWidget::eventFilter(object,event);
}

QModelIndexList FilterableListWidget::getNonHiddenSelectedRows()
{
  QModelIndexList non_hidden_list;
  for (const auto &selected_index : _table_view->selectionModel()->selectedRows(0))
  {
    if (!_table_view->isRowHidden(selected_index.row()))
    {
      non_hidden_list.append(selected_index);
    }
  }
  return non_hidden_list;
}


void FilterableListWidget::on_radioContains_toggled(bool checked)
{
    if(checked) {
        updateFilter();
        ui->lineEdit->setCompleter( nullptr );
        QSettings settings;
        settings.setValue("FilterableListWidget.searchFilter", "radioContains");
    }
}

void FilterableListWidget::on_radioRegExp_toggled(bool checked)
{
    if(checked) {
        updateFilter();
        ui->lineEdit->setCompleter( nullptr );
        QSettings settings;
        settings.setValue("FilterableListWidget.searchFilter", "radioRegExp");
    }
}

void FilterableListWidget::on_radioPrefix_toggled(bool checked)
{
    _completer_need_update = checked;

    if( checked )
    {
        _completer->clear();
        for (int row=0; row< rowCount(); row++)
        {
            auto item = _model->item(row,0);
            _completer->addToCompletionTree(item->text());
        }

        updateFilter();
        ui->lineEdit->setCompleter( _completer );
        QSettings settings;
        settings.setValue("FilterableListWidget.searchFilter", "radioPrefix");
    }
}

void FilterableListWidget::on_checkBoxCaseSensitive_toggled(bool checked)
{
    updateFilter();
}

void FilterableListWidget::on_lineEdit_textChanged(const QString &search_string)
{
    int item_count = rowCount();
    int visible_count = 0;
    bool updated = false;

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if( ui->checkBoxCaseSensitive->isChecked())
    {
        cs = Qt::CaseSensitive;
    }
    QRegExp regexp( search_string,  cs, QRegExp::Wildcard );
    QRegExpValidator v(regexp, 0);

    QStringList spaced_items = search_string.split(' ');

    for (int row=0; row < rowCount(); row++)
    {
        auto item = _model->item(row,0);
        QString name = item->text();
        int pos = 0;
        bool toHide = false;

        if( ui->radioRegExp->isChecked())
        {
            toHide = v.validate( name, pos ) != QValidator::Acceptable;
        }
        else if( ui->radioPrefix->isChecked())
        {
            toHide = !name.startsWith( search_string, cs ) ;
        }
        else if( ui->radioContains->isChecked())
        {
            for (const auto& item: spaced_items)
            {
                if( name.contains(item, cs) == false )
                {
                    toHide = true;
                    break;
                }
            }
        }
        if( !toHide ) visible_count++;

        if( toHide != _table_view->isRowHidden(row) ) updated = true;

        _table_view->setRowHidden(row, toHide );
    }
    ui->labelNumberDisplayed->setText( QString::number( visible_count ) + QString(" of ") + QString::number( item_count ) );

    if(updated){
        emit hiddenItemsChanged();
    }
}

void FilterableListWidget::on_pushButtonSettings_toggled(bool checked)
{
    ui->widgetOptions->setVisible(checked);
}

void FilterableListWidget::on_checkBoxHideSecondColumn_toggled(bool checked)
{
    if(checked){
        _table_view->hideColumn(1);
    }
    else{
        _table_view->showColumn(1);
    }
    emit hiddenItemsChanged();
}

void FilterableListWidget::removeSelectedCurves()
{
    QStringList names;

    auto selected_rows = getNonHiddenSelectedRows();
    for (int i = selected_rows.size() - 1; i >= 0; i--)
    {
        auto item = _model->item(selected_rows.at(i).row(), 0);
        names.push_back(item->text());
    }

    askToRemoveCurves(names);
}

void FilterableListWidget::askToRemoveCurves(QStringList names)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, tr("Warning"),
                                  tr("Do you really want to remove these data?\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No );

    if( reply == QMessageBox::Yes ) {
        for(const QString &name : names)
        {
            deleteCurve(name.toStdString());
        }

        // rebuild the tree model
        if( _completer_need_update )
        {
            _completer->clear();
            for (int row=0; row< rowCount(); row++)
            {
                auto item = _model->item(row);
                _completer->addToCompletionTree(item->text());
            }
        }
        updateFilter();
    }
}

void FilterableListWidget::removeRow(int row)
{
    _model->removeRow(row);
}


