#include "curvetree_view.h"
#include "curvelist_panel.h"
#include <QFontDatabase>

CurveTreeView::CurveTreeView(CurveListPanel* parent) : QTreeWidget(parent), CurvesView(parent)
{
    setColumnCount(2);
    setEditTriggers(NoEditTriggers);
    setDragEnabled(false);
    setDefaultDropAction(Qt::IgnoreAction);
    setDragDropOverwriteMode(false);
    setDragDropMode(NoDragDrop);
    viewport()->installEventFilter(this);
    setSelectionMode(ExtendedSelection);

    header()->setVisible(false);
    header()->setStretchLastSection(true);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

void CurveTreeView::addItem(const QString& item_name)
{
    auto parts = item_name.split('/', QString::SplitBehavior::SkipEmptyParts);
    if (parts.size() == 0)
    {
        return;
    }

    QTreeWidgetItem* tree_parent = this->invisibleRootItem();

    for (int i = 0; i < parts.size(); i++)
    {
        bool is_leaf = (i == parts.size() - 1);
        const auto& part = parts[ i ];

        QTreeWidgetItem* matching_child = nullptr;

        for (int c = 0; c < tree_parent->childCount(); c++)
        {
            QTreeWidgetItem* tree_child = tree_parent->child(c);
            if (tree_child->text(0) == part)
            {
                matching_child = tree_child;
                break;
            }
        }

        if (matching_child)
        {
            tree_parent = matching_child;
        }
        else
        {
            QTreeWidgetItem* child_item = new QTreeWidgetItem(tree_parent);
            child_item->setText(0, part);
            child_item->setText(1, "-");

            QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
            font.setPointSize(_point_size);
            child_item->setFont(0, font);

            font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
            font.setPointSize(_point_size);
            child_item->setFont(1, font);
            child_item->setTextAlignment(1, Qt::AlignRight);

            tree_parent = child_item;

            auto current_flag = child_item->flags();

            if (is_leaf)
            {
                child_item->setFlags(current_flag | Qt::ItemIsSelectable);
                child_item->setData(0, Qt::UserRole, item_name);
            }
            else
            {
                child_item->setFlags(current_flag & (~Qt::ItemIsSelectable));
            }
            // tree_parent->setSelectable(is_leaf);
        }
    }
}

void CurveTreeView::refreshColumns()
{
    treeVisitor([&](QTreeWidgetItem* item)
                {
                    item->sortChildren(0, Qt::AscendingOrder);
                } );
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // TODO emit updateFilter();
}

std::vector<std::string> CurveTreeView::getNonHiddenSelectedRows()
{
    std::vector<std::string> non_hidden_list;

    for (const auto& item : selectedItems())
    {
        non_hidden_list.push_back(
            item->data(0, Qt::UserRole).toString().toStdString());
    }
    return non_hidden_list;
}

void CurveTreeView::refreshFontSize()
{
    header()->setSectionResizeMode(0, QHeaderView::Fixed);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);

    treeVisitor([this](QTreeWidgetItem* item) {
        auto font = item->font(0);
        font.setPointSize(_point_size);
        item->setFont(0, font);
        font = item->font(1);
        font.setPointSize(_point_size);
        item->setFont(1, font);
    });

    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::Stretch);
}

bool CurveTreeView::applyVisibilityFilter(CurvesView::FilterType type,
                                          const QString& search_string)
{
    bool updated = false;
    _hidden_count = 0;
    QRegExp regexp(search_string, Qt::CaseInsensitive, QRegExp::Wildcard);
    QRegExpValidator v(regexp, nullptr);

    QStringList spaced_items = search_string.split(' ');

    auto hideFunc = [&](QTreeWidgetItem* item)
    {
        QString name = item->data(0, Qt::UserRole).toString();
        if( name.isEmpty() )
        {
            return; // not a leaf
        }
        bool toHide = false;

        if (search_string.isEmpty() == false)
        {
            if (type == REGEX)
            {
                int pos = 0;
                toHide = (v.validate(name, pos) != QValidator::Acceptable);
            }
            else if (type == CONTAINS)
            {
                for (const auto& item : spaced_items)
                {
                    if (name.contains(item, Qt::CaseInsensitive) == false)
                    {
                        toHide = true;
                        break;
                    }
                }
            }
        }
        if (toHide)
        {
            _hidden_count++;
        }

        if (toHide != item->isHidden())
        {
            updated = true;
        }

        item->setHidden(toHide);

        // hide the parent if necessary
        auto parent = item->parent();
        while( parent )
        {
            bool all_children_hidden = true;
            for (int c = 0; c < parent->childCount(); c++)
            {
                if( !parent->child(c)->isHidden() )
                {
                    all_children_hidden = false;
                    break;
                }
            }
            auto parent_hidden = parent->isHidden();
            if( all_children_hidden != parent_hidden)
            {
                parent->setHidden( all_children_hidden );
                parent = parent->parent();
            }
            else{
                break;
            }
        }
    };

   treeVisitor( hideFunc );
   //-------------

   return updated;
}

void CurveTreeView::treeVisitor(std::function<void(QTreeWidgetItem*)> visitor)
{
    std::function<void(QTreeWidgetItem*)> recursiveFunction;
    recursiveFunction = [&](QTreeWidgetItem* item)
    {
        visitor(item);
        for (int c = 0; c < item->childCount(); c++)
        {
            recursiveFunction(item->child(c));
        }
    };

    for (int c = 0; c < invisibleRootItem()->childCount(); c++)
    {
        recursiveFunction(invisibleRootItem()->child(c));
    }
}
