#include "curvetree_view.h"
#include <QFontDatabase>

CurveTreeView::CurveTreeView()
{

}

TreeItem *TreeItem::appendChild(const QString &name)
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

    auto res = _child_items_map.insert( {name, TreeItem(child_name, child_value, this)});
    return &(res.first->second);
}

TreeItem *TreeItem::findChild(const QString &name) {
    auto it = _child_items_map.find(name);
    if (it == _child_items_map.end()) {
        return nullptr;
    }
    return &(it->second);
}
