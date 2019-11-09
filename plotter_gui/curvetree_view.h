#ifndef CURVETREE_VIEW_H
#define CURVETREE_VIEW_H

#include <QWidget>
#include <QStandardItem>

class TreeItem {
   public:
    explicit TreeItem(QStandardItem* name_item,
                      QStandardItem* value_item,
                      TreeItem* parent)
        : _name_item(name_item), _value_item(value_item) {}

    TreeItem* appendChild(const QString& name);

    TreeItem* findChild(const QString& name);

    QStandardItem* nameItem() { return _name_item; }
    QStandardItem* valueItem() { return _value_item; }

   private:
    std::map<QString, TreeItem> _child_items_map;
    QStandardItem* _name_item;
    QStandardItem* _value_item;
    TreeItem* _parent;
};


class TreeModel : public QStandardItemModel {
   public:
    TreeModel(QStandardItemModel* parent_model)
        : QStandardItemModel(0, 2, parent_model),
          _root_tree_item(invisibleRootItem(), nullptr, nullptr) {}

    void clear() {
        QStandardItemModel::clear();
        _root_tree_item = TreeItem(invisibleRootItem(), nullptr, nullptr);
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
};


class CurveTreeView
{
   public:
    CurveTreeView();
};

#endif // CURVETREE_VIEW_H
