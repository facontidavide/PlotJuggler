/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CURVETREE_VIEW_H
#define CURVETREE_VIEW_H

#include "curvelist_view.h"
#include <QTreeWidget>
#include <functional>

class CurveTreeView : public QTreeWidget, public CurvesView
{
public:
  CurveTreeView(CurveListPanel* parent);

  void clear() override;

  void addItem(const QString& prefix, const QString& tree_name, const QString& plot_ID) override;

  void refreshColumns() override;

  std::vector<std::string> getSelectedNames() override;

  void refreshFontSize() override;

  bool applyVisibilityFilter(const QString& filter_string) override;

  bool eventFilter(QObject* object, QEvent* event) override;

  void removeCurve(const QString& name) override;

  std::pair<int, int> hiddenItemsCount() override
  {
    return { _hidden_count, _leaf_count };
  }

  void setViewResizeEnabled(bool) override
  {
  }

  virtual void hideValuesColumn(bool hide) override;

  void treeVisitor(std::function<void(QTreeWidgetItem*)> visitor);

  virtual void keyPressEvent(QKeyEvent*) override;

private:
  void expandChildren(bool expanded, QTreeWidgetItem* item);

  int _hidden_count = 0;
  int _leaf_count = 0;

  QTimer* _tooltip_timer = nullptr;
  QTreeWidgetItem* _tooltip_item = nullptr;
  QPoint _tooltip_pos;
};

#endif  // CURVETREE_VIEW_H
