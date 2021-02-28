#ifndef CURVE_SELECTOR_H
#define CURVE_SELECTOR_H

#include <QWidget>
#include <QAction>
#include <QListWidget>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QTableView>
#include <QItemSelection>

#include "transforms/custom_function.h"
#include "tree_completer.h"
#include "curvetree_view.h"
#include <array>

namespace Ui
{
class CurveListPanel;
}

class CurveListPanel : public QWidget
{
  Q_OBJECT

public:
  explicit CurveListPanel(const CustomPlotMap& mapped_math_plots, QWidget* parent);

  ~CurveListPanel() override;

  void clear();

  void addCurve(const QString& item_name);

  void addCustom(const QString& item_name);

  void refreshColumns();

  void removeCurve(const std::string& name);

  void rebuildEntireList(const std::vector<std::string>& names);

  void updateFilter();

  void changeFontSize(int point_size);

  bool is2ndColumnHidden() const;

  void update2ndColumnValues(double time, std::unordered_map<std::string, PlotData>* numeric_data);

  virtual void keyPressEvent(QKeyEvent* event) override;

private slots:

  void on_lineEditFilter_textChanged(const QString& search_string);

  void removeSelectedCurves();

  void on_buttonAddCustom_clicked();

  void on_buttonEditCustom_clicked();

  void onCustomSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  void on_checkBoxShowValues_toggled(bool show);

public slots:

  std::vector<std::string> getSelectedNames() const;

  void clearSelections();

  void on_stylesheetChanged(QString theme);

  void refreshValues();

private:
  Ui::CurveListPanel* ui;

  void updateTreeModel();

  CurveTableView* _custom_view;
  CurveTreeView* _tree_view;

  double _tracker_time = 0;
  std::unordered_map<std::string, PlotData>* _numeric_data = nullptr;

  const CustomPlotMap& _custom_plots;

  QString _style_dir;

  bool _column_width_dirty;

signals:

  void hiddenItemsChanged();

  void createMathPlot(const std::string& linked_plot);
  void editMathPlot(const std::string& plot_name);
  void refreshMathPlot(const std::string& curve_name);

  void deleteCurves(const std::vector<std::string>& curve_names);
};

#endif  // CURVE_SELECTOR_H
