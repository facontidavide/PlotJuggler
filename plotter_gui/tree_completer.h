#ifndef TREE_COMPLETER_H
#define TREE_COMPLETER_H

#include <QString>
#include <QHash>
#include <QVariant>
#include <QList>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QCompleter>

class TreeItem
{
public:
    explicit TreeItem(QStandardItem* item): _item(item)
    {

    }

    TreeItem* appendChild(const QString& name, QStandardItem* item)
    {
        return &(_child_items_map.insert(name, TreeItem(item) ).value());
    }

    TreeItem * findChild(const QString& name)
    {
        auto it = _child_items_map.find(name);
        if( it == _child_items_map.end())
        {
            return nullptr;
        }
        return &(it.value());
    }

    QStandardItem* standardItem() { return _item; }

private:
    QHash<QString, TreeItem> _child_items_map;
    QStandardItem* _item;
};

class TreeModelCompleter : public QCompleter
{

public:
    TreeModelCompleter(QObject *parent = 0):
        QCompleter(parent),
        _model(new QStandardItemModel()),
        _root_tree_item(nullptr)
    {
        setModelSorting( QCompleter::CaseInsensitivelySortedModel );
        setModel(_model);
        _root_tree_item = TreeItem(_model->invisibleRootItem());
    }

    QStandardItemModel* treeModel() { return _model; }

    void clear()
    {
        _model->clear();
        _root_tree_item = TreeItem(_model->invisibleRootItem());
    }

    QStringList splitPath(const QString &path) const override {
        return path.split('/');
    }

    QString pathFromIndex(const QModelIndex &index) const override
    {
        QStringList dataList;
        for (QModelIndex it = index; it.isValid(); it = it.parent())
        {
            dataList.prepend(model()->data(it, completionRole()).toString());
        }
        return dataList.join('/');
    }

    void addToCompletionTree(const QString &name)
    {
        auto parts = name.split('/');
        if( parts.size() == 0 )
        {
            return;
        }

        TreeItem* tree_parent = & _root_tree_item;
        QStandardItem *item_parent = tree_parent->standardItem();

        for (const auto& part: parts)
        {
            TreeItem* matching_child = tree_parent->findChild( part );
            if(matching_child)
            {
                tree_parent = matching_child;
                item_parent = matching_child->standardItem();
            }
            else
            {
                QStandardItem* item = new QStandardItem( part );
                item_parent->appendRow(item);
                tree_parent = tree_parent->appendChild(part, item);
                item_parent = item;
            }
        }
    }

private:

    QStandardItemModel* _model;
    TreeItem _root_tree_item;
};


#endif // TREE_COMPLETER_H
