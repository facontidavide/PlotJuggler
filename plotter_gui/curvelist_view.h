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
    CurvesView();

    virtual void addItem(const QString& item_name) = 0;

    virtual std::vector<std::string> getNonHiddenSelectedRows() = 0;

    enum FilterType{ CONTAINS, REGEX };

    virtual bool applyVisibilityFilter(FilterType type, const QString& filter_string) = 0;

    virtual void refreshFontSize() = 0;

    virtual void refreshColumns() = 0;

    virtual void hideValuesColumn(bool hide) = 0;

    bool eventFilter(QObject* object, QEvent* event);

    virtual std::pair<int,int> hiddenItemsCount() = 0;


    QAbstractItemView* view() // ugly code!
    {
        return dynamic_cast<QAbstractItemView*>(this);
    }

   protected:
    int _point_size;
    QPoint _drag_start_pos;
    bool _newX_modifier = false;
    bool _dragging = false;
};

class CurveTableView : public QTableView, public CurvesView
{
   public:
    CurveTableView(QStandardItemModel* model, QWidget* parent);

    void addItem(const QString& item_name);

    void refreshColumns() override;

    std::vector<std::string> getNonHiddenSelectedRows() override;

    void refreshFontSize() override;

    bool applyVisibilityFilter(FilterType type, const QString& filter_string) override;

    bool eventFilter(QObject* object, QEvent* event) override
    {
        bool ret = CurvesView::eventFilter(object, event);
        if(!ret)
        {
            return QTableView::eventFilter(object,event);
        }
        else{
            return true;
        }
    }

    virtual std::pair<int,int> hiddenItemsCount()
    {
        return { _hidden_count, model()->rowCount() };
    }

    virtual void hideValuesColumn(bool hide) override
    {
        if(hide){
            hideColumn(1);
        }
        else  {
            showColumn(1);
        }
    }
   private:
    QStandardItemModel* _model;
    int _hidden_count = 0;
};

#endif  // CURVELIST_VIEW_H
