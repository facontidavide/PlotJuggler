#ifndef CURVELIST_VIEW_H
#define CURVELIST_VIEW_H

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHeaderView>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>
#include <vector>

#include "PlotJuggler/alphanum.hpp"

class SortedTableItem : public QStandardItem
{
   public:
    SortedTableItem(const QString& name)
        : QStandardItem(name), str(name.toStdString())
    {
    }

    bool operator<(const SortedTableItem& other) const
    {
        return doj::alphanum_impl(this->str.c_str(), other.str.c_str()) < 0;
    }

   protected:
    std::string str;
};

class CurvesView
{
   public:
    CurvesView()
    {
        QSettings settings;
        _point_size =
            settings.value("FilterableListWidget/table_point_size", 9).toInt();
    }

    virtual void addItem(const QString& item_name) = 0;

    virtual std::vector<std::string> getNonHiddenSelectedRows() = 0;

    enum FilterType{ CONTAINS, REGEX };

    virtual bool applyVisibilityFilter(FilterType type, const QString& filter_string) = 0;

    virtual void refreshFontSize() = 0;

    virtual void refreshColumns() = 0;

    bool eventFilter(QObject* object, QEvent* event);

    QAbstractItemView* view() // ugly code!
    {
        return dynamic_cast<QAbstractItemView*>(this);
    }

   protected:
    int _point_size;
    QPoint _drag_start_pos;
    bool _newX_modifier;
    bool _dragging;
};

class CurveTableView : public QTableView, public CurvesView
{
   public:
    CurveTableView(QStandardItemModel* model, QWidget* parent);

    void addItem(const QString& item_name)
    {
        if (_model->findItems(item_name).size() > 0)
        {
            return;
        }

        auto item = new SortedTableItem(item_name);
        QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        font.setPointSize(_point_size);
        item->setFont(font);
        const int row = model()->rowCount();
        _model->setRowCount(row + 1);
        _model->setItem(row, 0, item);

        auto val_cell = new QStandardItem("-");
        val_cell->setTextAlignment(Qt::AlignRight);
        val_cell->setFlags(Qt::NoItemFlags | Qt::ItemIsEnabled);
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(_point_size);
        val_cell->setFont(font);
        val_cell->setFlags(Qt::NoItemFlags);

        _model->setItem(row, 1, val_cell);
    }

    void refreshColumns() override;

    std::vector<std::string> getNonHiddenSelectedRows() override;

    void refreshFontSize() override
    {
        horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);

        verticalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        verticalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);

        for (int row = 0; row < _model->rowCount(); row++)
        {
            for (int col = 0; col < 2; col++)
            {
                auto item = _model->item(row, col);
                auto font = item->font();
                font.setPointSize(_point_size);
                item->setFont(font);
            }
        }

        horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    bool applyVisibilityFilter(FilterType type, const QString& filter_string) override;

    bool eventFilter(QObject* object, QEvent* event) override
    {
        bool ret = CurvesView::eventFilter(object, event);
        if(!ret)
        {
            return QTableView::eventFilter(object,event);
        }
    }

   private:
    QStandardItemModel* _model;
};

#endif  // CURVELIST_VIEW_H
