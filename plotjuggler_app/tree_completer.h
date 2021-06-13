#ifndef TREE_COMPLETER_H
#define TREE_COMPLETER_H

#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QVariant>
#include <QFont>
#include <QFontDatabase>
#include <map>
#include "PlotJuggler/alphanum.hpp"
/*/
class TreeItem {
 public:
  explicit TreeItem(QStandardItem* name_item, QStandardItem* value_item)
      : _name_item(name_item), _value_item(value_item) {}

  TreeItem* appendChild(const QString& name)
  {
    auto child_name = new QStandardItem(name);
    auto child_value = new QStandardItem("-");

    child_value->setSelectable(false);
    child_value->setTextAlignment(Qt::AlignRight);
    child_value->setFlags( Qt::NoItemFlags | Qt::ItemIsEnabled );
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    child_value->setFont( font );
    child_value->setFlags(Qt::NoItemFlags);

    QList<QStandardItem*> columns;
    columns << child_name << child_value;
    _name_item->appendRow(columns);

    auto res =
        _child_items_map.insert(std::make_pair(name, TreeItem(child_name, child_value)));
    return &(res.first->second);
  }

  TreeItem* findChild(const QString& name) {
    auto it = _child_items_map.find(name);
    if (it == _child_items_map.end()) {
      return nullptr;
    }
    return &(it->second);
  }

  QStandardItem* nameItem() { return _name_item; }
  QStandardItem* valueItem() { return _value_item; }

 private:
  std::map<QString, TreeItem> _child_items_map;
  QStandardItem* _name_item;
  QStandardItem* _value_item;
};

class TreeModel : public QAbstractItemModel {
 public:
  TreeModel(QStandardItemModel* parent_model)
      : QStandardItemModel(0, 2, parent_model),
        _root_tree_item(invisibleRootItem(), nullptr),
        _parent_model(parent_model) {}

  void clear() {
    QStandardItemModel::clear();
    _root_tree_item = TreeItem(invisibleRootItem(), nullptr);
  }

  void addToTree(const QString& name, int reference_row) {
    auto parts = name.split('/', QString::SplitBehavior::SkipEmptyParts);
    if (parts.size() == 0) {
      return;
    }

    TreeItem* tree_parent = &_root_tree_item;

    for (int i = 0; i < parts.size(); i++) {
      bool is_leaf = (i == parts.size() - 1);
      const auto& part = parts[i];

      TreeItem* matching_child = tree_parent->findChild(part);
      if (matching_child) {
        tree_parent = matching_child;
      } else {

        tree_parent = tree_parent->appendChild(part);
        tree_parent->nameItem()->setSelectable(is_leaf);
      }
    }
  }

 private:
  TreeItem _root_tree_item;
  QStandardItemModel* _parent_model;
};*/

#endif  // TREE_COMPLETER_H
