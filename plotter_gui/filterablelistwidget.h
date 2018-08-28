#ifndef CURVE_SELECTOR_H
#define CURVE_SELECTOR_H

#include <QWidget>
#include <QAction>
#include <QListWidget>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QTableView>

#include "math_plot.h"
#include "tree_completer.h"

class CustomSortedTableItem;

namespace Ui {
class FilterableListWidget;
}

class FilterableListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilterableListWidget(const std::unordered_map<std::string, MathPlotPtr>& mapped_math_plots, QWidget *parent = 0);
    ~FilterableListWidget();

    int rowCount() const;

    void clear();

    void addItem(const QString& item_name);

    void refreshColumns();

    int findRowByName(const std::string &text) const;

    void removeRow(int row);

    void rebuildEntireList(const std::vector<std::string> &names);

    void updateFilter();

    QStandardItemModel *getTable() const
    {
        return _model;
    }

    QTableView *getView() const
    {
        return _table_view;
    }

    bool is2ndColumnHidden() const
    {
        return getView()->isColumnHidden(1);
    }

    virtual void keyPressEvent(QKeyEvent * event) override;

private slots:

    void on_radioContains_toggled(bool checked);

    void on_radioRegExp_toggled(bool checked);

    void on_radioPrefix_toggled(bool checked);

    void on_checkBoxCaseSensitive_toggled(bool checked);

    void on_lineEdit_textChanged(const QString &search_string);

    void on_pushButtonSettings_toggled(bool checked);

    void on_checkBoxHideSecondColumn_toggled(bool checked);

    void removeSelectedCurves();
    void askToRemoveCurves(QStringList names);

private:

    Ui::FilterableListWidget *ui;

    QPoint _drag_start_pos;

    bool _newX_modifier, _dragging;

    TreeModelCompleter* _completer;

    bool eventFilter(QObject *object, QEvent *event);

    void updateTreeModel();
    
    QModelIndexList getNonHiddenSelectedRows();

    bool _completer_need_update;

    QStandardItemModel* _model;

    QTableView* _table_view;

    const std::unordered_map<std::string, MathPlotPtr>& _mapped_math_plots;

signals:

    void hiddenItemsChanged();

    void createMathPlot(const std::string& linked_plot);
    void editMathPlot(const std::string& plot_name);
    void refreshMathPlot(const std::string& curve_name);
    void deleteCurve(const std::string& curve_name);

};

#endif // CURVE_SELECTOR_H
