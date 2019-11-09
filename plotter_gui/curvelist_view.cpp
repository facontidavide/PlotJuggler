#include "curvelist_view.h"
#include <QApplication>
#include <QDrag>
#include <QMessageBox>
#include <QMimeData>
#include <QDebug>

CurveTableView::CurveTableView(QStandardItemModel *model, QWidget *parent)
    : QTableView(parent), _model(model)
{
    setEditTriggers(NoEditTriggers);
    setDragEnabled(false);
    setDefaultDropAction(Qt::IgnoreAction);
    setDragDropOverwriteMode(false);
    setDragDropMode(NoDragDrop);
    viewport()->installEventFilter(this);
    setModel(model);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setShowGrid(false);
}

void CurveTableView::refreshColumns()
{
    sortByColumn(0, Qt::AscendingOrder);
    horizontalHeader()->setSectionResizeMode(0,
                                             QHeaderView::ResizeToContents);
    // TODO emit updateFilter();
}

std::vector<std::string> CurveTableView::getNonHiddenSelectedRows()
{
    std::vector<std::string> non_hidden_list;

    for (const auto& selected_index : selectionModel()->selectedRows(0))
    {
        if (!isRowHidden(selected_index.row()))
        {
            auto item = _model->item(selected_index.row(), 0);
            non_hidden_list.push_back(item->text().toStdString());
        }
    }
    return non_hidden_list;
}

bool CurveTableView::applyVisibilityFilter(CurvesView::FilterType type, const QString &search_string)
{
    bool updated = false;
    int visible_count = 0;
    QRegExp regexp( search_string, Qt::CaseInsensitive, QRegExp::Wildcard );
    QRegExpValidator v(regexp, nullptr);

    QStringList spaced_items = search_string.split(' ');

    for (int row=0; row < model()->rowCount(); row++)
    {
        auto item = _model->item(row,0);
        QString name = item->text();
        int pos = 0;
        bool toHide = false;

        if( search_string.isEmpty() == false )
        {
            if( type == REGEX)
            {
                toHide = v.validate( name, pos ) != QValidator::Acceptable;
            }
            else if( type == CONTAINS)
            {
                for (const auto& item: spaced_items)
                {
                    if( name.contains(item, Qt::CaseInsensitive) == false )
                    {
                        toHide = true;
                        break;
                    }
                }
            }
        }
        if( !toHide ) visible_count++;

        if( toHide != isRowHidden(row) ){
            updated = true;
        }

        setRowHidden(row, toHide );
    }
    return updated;
}

bool CurvesView::eventFilter(QObject *object, QEvent *event)
{
    QAbstractItemView* table_widget = dynamic_cast<QAbstractItemView*>(object);

    bool shift_modifier_pressed =
            (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier);
    bool ctrl_modifier_pressed =
            (QGuiApplication::keyboardModifiers() == Qt::ControlModifier);

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event);

        _dragging = false;
        _drag_start_pos = mouse_event->pos();

        // TODO
        //            if( !shift_modifier_pressed && !ctrl_modifier_pressed &&
        //            mouse_event->button() != Qt::RightButton  )
        //            {
        //                if( obj == ui->tableView)
        //                {
        //                    ui->tableViewCustom->clearSelection() ;
        //                }
        //                if( obj == ui->tableViewCustom)
        //                {
        //                    ui->tableView->clearSelection() ;
        //                }
        //            }

        if (mouse_event->button() == Qt::LeftButton)
        {
            _newX_modifier = false;
        }
        else if (mouse_event->button() == Qt::RightButton)
        {
            _newX_modifier = true;
        }
        else
        {
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouse_event = static_cast<QMouseEvent *>(event);
        double distance_from_click =
            (mouse_event->pos() - _drag_start_pos).manhattanLength();

        if ((mouse_event->buttons() == Qt::LeftButton ||
             mouse_event->buttons() == Qt::RightButton) &&
            distance_from_click >= QApplication::startDragDistance() &&
            !_dragging)
        {
            _dragging = true;
            QDrag *drag = new QDrag(table_widget);
            QMimeData *mimeData = new QMimeData;

            QByteArray mdata;
            QDataStream stream(&mdata, QIODevice::WriteOnly);

            for (const auto &curve_name : getNonHiddenSelectedRows())
            {
                stream << QString::fromStdString(curve_name);
            }

            if (!_newX_modifier)
            {
                mimeData->setData("curveslist/add_curve", mdata);
            }
            else
            {
                if (getNonHiddenSelectedRows().size() != 2)
                {
                    if (getNonHiddenSelectedRows().size() >= 1)
                    {
                        QMessageBox::warning(
                            table_widget, "New in version 2.3+",
                            "To create a new XY curve, you must select two "
                            "timeseries and "
                            "drag&drop them using the RIGHT mouse button.",
                            QMessageBox::Ok);
                    }
                    return true;
                }
                mimeData->setData("curveslist/new_XY_axis", mdata);

                QPixmap cursor(QSize(160, 30));
                cursor.fill(Qt::transparent);

                QPainter painter;
                painter.begin(&cursor);
                painter.setPen(QColor(22, 22, 22));

                QString text("Create a XY curve");
                painter.setFont(QFont("Arial", 14));

                painter.setBackground(Qt::transparent);
                painter.setPen(table_widget->palette().foreground().color());
                painter.drawText(QRect(0, 0, 160, 30),
                                 Qt::AlignHCenter | Qt::AlignVCenter, text);
                painter.end();

                drag->setDragCursor(cursor, Qt::MoveAction);
            }

            for(const QString& format: mimeData->formats())
            {
                qDebug() << format;
            }
            drag->setMimeData(mimeData);
            drag->exec(Qt::CopyAction | Qt::MoveAction);
        }
        return true;
    }
    else if (event->type() == QEvent::Wheel)
    {
        QWheelEvent *wheel_event = dynamic_cast<QWheelEvent *>(event);
        int prev_size = _point_size;
        if (ctrl_modifier_pressed)
        {
            if (_point_size > 6 && wheel_event->delta() < 0)
            {
                _point_size--;
            }
            else if (_point_size < 14 && wheel_event->delta() > 0)
            {
                _point_size++;
            }
            if (_point_size != prev_size)
            {
                refreshFontSize();
                QSettings settings;
                settings.setValue("FilterableListWidget/table_point_size",
                                  _point_size);
            }
            return true;
        }
    }

    return false;
}
