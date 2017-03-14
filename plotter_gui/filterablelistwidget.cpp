#include "filterablelistwidget.h"
#include "ui_filterablelistwidget.h"
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

FilterableListWidget::FilterableListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterableListWidget)
{
    ui->setupUi(this);
    ui->tableWidget->viewport()->installEventFilter( this );

   table()->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   table()->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   table()->horizontalHeader()->resizeSection(1, 120);

    for( int i=0; i< ui->gridLayoutSettings->count(); i++)
    {
        QLayoutItem* item = ui->gridLayoutSettings->itemAt(i);
        if(item)
        {
            QWidget* widget = item->widget();
            if(widget) {
                widget->setMaximumHeight( 0 );
                widget->setVisible( false );
            }
        }
    }
}

FilterableListWidget::~FilterableListWidget()
{
    delete ui;
}

int FilterableListWidget::rowCount() const
{
    return table()->rowCount();
}

void FilterableListWidget::clear()
{
    table()->clear();
    ui->labelNumberDisplayed->setText( "0 of 0");
}

void FilterableListWidget::addItem(QTableWidgetItem *item)
{
    int row = rowCount();
    table()->setRowCount(row+1);
    table()->setItem(row, 0, item);

    auto val_cell = new QTableWidgetItem("-");
    val_cell->setTextAlignment(Qt::AlignRight);
    val_cell->setFlags( Qt::NoItemFlags | Qt::ItemIsEnabled );
    val_cell->setFont(  QFontDatabase::systemFont(QFontDatabase::FixedFont) );

    table()->setItem(row, 1, val_cell );
}


QList<int>
FilterableListWidget::findRowsByName(const QString &text) const
{
    QList<int> output;
    QList<QTableWidgetItem*> item_list = table()->findItems( text, Qt::MatchExactly);
    for(QTableWidgetItem* item : item_list)
    {
        if(item->column() == 0) {
            output.push_back( item->row() );
        }
    }
    return output;
}

const QTableWidget *FilterableListWidget::table() const
{
    return ui->tableWidget;
}

QTableWidget *FilterableListWidget::table()
{
    return ui->tableWidget;
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
    if(event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
        if(mouse_event->button() == Qt::LeftButton )
        {
            _newX_modifier = false;
            _drag_start_pos = mouse_event->pos();
        }
        else if(mouse_event->button() == Qt::RightButton )
        {
            _newX_modifier = true;
            _drag_start_pos = mouse_event->pos();
        }
    }
    else if(event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent*>(event);
        double distance_from_click = (mouse_event->pos() - _drag_start_pos).manhattanLength();

        if ((mouse_event->buttons() == Qt::LeftButton || mouse_event->buttons() == Qt::RightButton) &&
             distance_from_click >= QApplication::startDragDistance())
        {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;

            QByteArray mdata;
            QDataStream stream(&mdata, QIODevice::WriteOnly);

            for(QTableWidgetItem* item: table()->selectedItems()) {
                stream << item->text();
            }
            if( _newX_modifier )
            {
                if( table()->selectedItems().size() == 1)
                {
                    mimeData->setData("curveslist/new_X_axis", mdata);
                    QPixmap icon(":/icons/resources/resize_vertical.png");
                    drag->setDragCursor(icon, Qt::MoveAction);
                }
                else{
                    //abort
                    QWidget::eventFilter(object,event);
                }
            }
            else{
                mimeData->setData("curveslist/add_curve", mdata);
            }

            drag->setMimeData(mimeData);
            drag->exec(Qt::CopyAction | Qt::MoveAction);
        }
    }
    return QWidget::eventFilter(object,event);
}


void FilterableListWidget::on_radioContains_toggled(bool checked)
{
    if(checked)
    {
        ui->radioRegExp->setChecked( false);
        on_lineEdit_textChanged( ui->lineEdit->text() );
    }
}

void FilterableListWidget::on_radioRegExp_toggled(bool checked)
{
    if(checked)
    {
        ui->radioContains->setChecked( false);
        on_lineEdit_textChanged( ui->lineEdit->text() );
    }
}

void FilterableListWidget::on_checkBoxCaseSensitive_toggled(bool checked)
{
    on_lineEdit_textChanged( ui->lineEdit->text() );
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

    for (int row=0; row< rowCount(); row++)
    {
        QTableWidgetItem* item = table()->item(row,0);
        QString name = item->text();
        int pos = 0;
        bool toHide = false;

        if( ui->radioRegExp->isChecked())
            toHide = v.validate( name, pos ) != QValidator::Acceptable;
        else{
            QStringList items = search_string.split(' ');
            for (int i=0; i< items.size(); i++)
            {
                if( name.contains(items[i], cs) == false )
                {
                    toHide = true;
                }
            }
        }
        if( !toHide ) visible_count++;

        if( toHide != table()->isRowHidden(row) ) updated = true;

        table()->setRowHidden(row, toHide );
    }
    ui->labelNumberDisplayed->setText( QString::number( visible_count ) + QString(" of ") + QString::number( item_count ) );

    if(updated){
        emit hiddenItemsChanged();
    }
}

void FilterableListWidget::on_pushButtonSettings_toggled(bool checked)
{
    for( int i=0; i< ui->gridLayoutSettings->count(); i++)
    {
        QLayoutItem* item = ui->gridLayoutSettings->itemAt(i);
        if(item)
        {
            QWidget* widget = item->widget();
            if(widget)
            {
                widget->setMaximumHeight( checked ? 25:0 );
                widget->setVisible( checked );

            }
        }
    }
}

void FilterableListWidget::on_checkBoxHideSecondColumn_toggled(bool checked)
{
    if(checked){
        table()->hideColumn(1);
        emit hiddenItemsChanged();
    }
    else{
        table()->showColumn(1);
        emit hiddenItemsChanged();
    }
}

void FilterableListWidget::removeSelectedCurves()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, tr("Warning"),
                                  tr("Do you really want to remove these data?\n"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No );

    if( reply == QMessageBox::Yes ) {

        while( table()->selectedItems().size() > 0 )
        {
            QTableWidgetItem* item = table()->selectedItems().first();
            emit deleteCurve( item->text() );
        }
    }
}
