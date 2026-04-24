/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "topic_panel.h"

#include "../format_utils.h"

#include <algorithm>

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

// Column indices.
static constexpr int kColName = 0;
static constexpr int kColSize = 1;

// Sorts numerically by the raw value stored in Qt::UserRole rather than by
// display text (so "1.2 MB" doesn't sort lexicographically as less than "900 KB").
namespace
{
class NumericTableItem : public QTableWidgetItem
{
public:
  using QTableWidgetItem::QTableWidgetItem;

  bool operator<(const QTableWidgetItem& other) const override
  {
    return data(Qt::UserRole).toLongLong() < other.data(Qt::UserRole).toLongLong();
  }
};
}  // namespace

TopicPanel::TopicPanel(QWidget* parent) : QWidget(parent)
{
  header_ = new QLabel("Topics", this);
  auto header_font = header_->font();
  header_font.setBold(true);
  header_->setFont(header_font);

  filter_ = new QLineEdit(this);
  filter_->setPlaceholderText("Filter…");
  connect(filter_, &QLineEdit::textChanged, this, &TopicPanel::applyFilter);

  regex_btn_ = new QPushButton(".*", this);
  regex_btn_->setCheckable(true);
  regex_btn_->setToolTip("Use regular expression");
  regex_btn_->setFixedSize(24, 24);
  auto regex_font = regex_btn_->font();
  regex_font.setBold(true);
  regex_btn_->setFont(regex_font);
  regex_btn_->setStyleSheet("QPushButton { padding: 0px; }");
  connect(regex_btn_, &QPushButton::toggled, this, &TopicPanel::applyFilter);

  auto* filter_row = new QHBoxLayout();
  filter_row->addWidget(filter_);
  filter_row->addWidget(regex_btn_);

  table_ = new QTableWidget(this);
  table_->setColumnCount(2);
  table_->setHorizontalHeaderLabels({ "Name", "Size" });
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->setVisible(false);
  table_->setSortingEnabled(true);
  table_->sortByColumn(kColName, Qt::AscendingOrder);
  table_->horizontalHeader()->setStretchLastSection(false);
  table_->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(kColSize, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setHighlightSections(false);
  connect(table_, &QTableWidget::itemSelectionChanged, this, &TopicPanel::onSelectionChanged);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(header_);
  layout->addLayout(filter_row);
  layout->addWidget(table_);
  setLayout(layout);
}

void TopicPanel::setCurrentSequence(const QString& sequence_name)
{
  current_sequence_ = sequence_name;
}

void TopicPanel::populateTopics(const std::vector<TopicInfo>& infos)
{
  // Sort client-side: the server's topic iteration order is not stable
  // (endpoints come from concurrent chunk queries), so the list would
  // otherwise visibly reshuffle on every re-fetch.
  std::vector<TopicInfo> sorted = infos;
  std::sort(sorted.begin(), sorted.end(),
            [](const TopicInfo& a, const TopicInfo& b) { return a.topic_name < b.topic_name; });

  // Sorting is disabled during population so setRowCount + setItem don't
  // fight the header's current sort order.
  table_->setSortingEnabled(false);
  table_->setRowCount(static_cast<int>(sorted.size()));

  for (int i = 0; i < static_cast<int>(sorted.size()); ++i)
  {
    const auto& info = sorted[static_cast<size_t>(i)];
    const QString name = QString::fromStdString(info.topic_name);

    auto* name_item = new QTableWidgetItem(name);
    table_->setItem(i, kColName, name_item);

    auto* size_item = new NumericTableItem(formatBytes(info.total_size_bytes));
    size_item->setData(Qt::UserRole, static_cast<qlonglong>(info.total_size_bytes));
    size_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, kColSize, size_item);
  }

  table_->setSortingEnabled(true);

  if (sorted.empty())
  {
    header_->setText("Topics (none — click a topic manually)");
  }
  else
  {
    header_->setText(QString("Topics (%1)").arg(sorted.size()));
  }
  applyFilter();
}

void TopicPanel::setLoading(bool loading)
{
  header_->setText(loading ? "Topics (loading…)" : "Topics");
  table_->setEnabled(!loading);
}

void TopicPanel::clear()
{
  table_->setRowCount(0);
  header_->setText("Topics");
  current_sequence_.clear();
}

void TopicPanel::onSelectionChanged()
{
  if (current_sequence_.isEmpty())
  {
    return;
  }
  QStringList selected;
  // selectionModel() reports the full grid of selected cells; dedupe by row
  // and read the Name column only.
  const auto rows = table_->selectionModel()->selectedRows(kColName);
  selected.reserve(rows.size());
  for (const auto& idx : rows)
  {
    auto* item = table_->item(idx.row(), kColName);
    if (item)
    {
      selected.append(item->text());
    }
  }
  emit topicsSelected(current_sequence_, selected);
}

void TopicPanel::applyFilter()
{
  const QString pattern = filter_->text();
  const bool use_regex = regex_btn_->isChecked();

  QRegularExpression re;
  if (use_regex)
  {
    re.setPattern(pattern);
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    if (!re.isValid())
    {
      re.setPattern(QRegularExpression::escape(pattern));
    }
  }

  for (int row = 0; row < table_->rowCount(); ++row)
  {
    auto* name_item = table_->item(row, kColName);
    if (!name_item)
    {
      continue;
    }
    const QString name = name_item->text();
    const bool matches =
        use_regex ? re.match(name).hasMatch() : name.contains(pattern, Qt::CaseInsensitive);
    table_->setRowHidden(row, !matches);
  }
}
