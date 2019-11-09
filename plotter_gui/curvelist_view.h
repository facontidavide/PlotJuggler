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
#include <QTableWidget>
#include <vector>

#include "PlotJuggler/alphanum.hpp"

class CurveListPanel;

template <typename ItemType>
class SortedTableItem : public ItemType
{
   public:
    SortedTableItem(const QString& name)
        : ItemType(name), str(name.toStdString())
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
    CurvesView(CurveListPanel* parent): _parent_panel(parent) {}

    virtual void clear() = 0;

    virtual void addItem(const QString& item_name) = 0;

    virtual std::vector<std::string> getSelectedNames() = 0;

    enum FilterType{ CONTAINS, REGEX };

    virtual bool applyVisibilityFilter(FilterType type, const QString& filter_string) = 0;

    virtual void refreshFontSize() = 0;

    virtual void refreshColumns() = 0;

    virtual void hideValuesColumn(bool hide) = 0;

    bool eventFilterBase(QObject* object, QEvent* event);

    virtual std::pair<int,int> hiddenItemsCount() = 0;

    virtual void removeCurve(const QString& name) = 0;

    void setFontSize(int size)
    {
        _point_size = size;
        refreshFontSize();
    }

   protected:
    int _point_size = 9;
    QPoint _drag_start_pos;
    bool _newX_modifier = false;
    bool _dragging = false;
    CurveListPanel* _parent_panel;
};

class CurveTableView : public QTableWidget, public CurvesView
{
   public:
    CurveTableView(CurveListPanel* parent);

    void clear() override
    {
        setRowCount(0);
    }

    void addItem(const QString& item_name);

    void refreshColumns() override;

    std::vector<std::string> getSelectedNames() override;

    void refreshFontSize() override;

    void removeCurve(const QString& name) override;

    bool applyVisibilityFilter(FilterType type, const QString& filter_string) override;

    bool eventFilter(QObject* object, QEvent* event) override
    {
        bool ret = CurvesView::eventFilterBase(object, event);
        if(!ret)
        {
            return QWidget::eventFilter(object,event);
        }
        else{
            return true;
        }
    }

    virtual std::pair<int,int> hiddenItemsCount()
    {
        return { _hidden_count, model()->rowCount() };
    }

    virtual void hideValuesColumn(bool hide) override;
   private:
    int _hidden_count = 0;
};

#endif  // CURVELIST_VIEW_H
