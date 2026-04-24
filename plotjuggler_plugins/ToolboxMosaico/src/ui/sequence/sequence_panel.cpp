/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "sequence_panel.h"

#include "../format_utils.h"
#include "../picker/sequence_picker_widget.h"
#include "../colors.h"

#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimeZone>
#include <QVBoxLayout>

#include <climits>
#include <limits>
#include <set>

// Column indices.
static constexpr int kColName = 0;
static constexpr int kColDate = 1;
static constexpr int kColSize = 2;

// Custom QTableWidgetItem that sorts numerically by the raw value
// stored in Qt::UserRole rather than by display text.
class NumericTableItem : public QTableWidgetItem
{
public:
  using QTableWidgetItem::QTableWidgetItem;

  bool operator<(const QTableWidgetItem& other) const override
  {
    return data(Qt::UserRole).toLongLong() < other.data(Qt::UserRole).toLongLong();
  }
};

SequencePanel::SequencePanel(QWidget* parent) : QWidget(parent)
{
  header_ = new QLabel("Sequences", this);
  auto header_font = header_->font();
  header_font.setBold(true);
  header_->setFont(header_font);

  filter_ = new QLineEdit(this);
  filter_->setPlaceholderText("Filter\u2026");
  connect(filter_, &QLineEdit::textChanged, this, &SequencePanel::applyFilter);

  regex_btn_ = new QPushButton(".*", this);
  regex_btn_->setCheckable(true);
  regex_btn_->setToolTip("Use regular expression");
  regex_btn_->setFixedSize(24, 24);
  auto regex_font = regex_btn_->font();
  regex_font.setBold(true);
  regex_btn_->setFont(regex_font);
  // Global QPushButton QSS has 6px padding which clips text on a 24x24 button.
  regex_btn_->setStyleSheet("QPushButton { padding: 0px; }");
  connect(regex_btn_, &QPushButton::toggled, this, &SequencePanel::applyFilter);

  auto* filter_row = new QHBoxLayout();
  filter_row->addWidget(filter_);
  filter_row->addWidget(regex_btn_);

  picker_ = new SequencePickerWidget(this);
  connect(picker_, &SequencePickerWidget::filterChanged, this, &SequencePanel::onFilterChanged);

  table_ = new QTableWidget(this);
  table_->setColumnCount(3);
  table_->setHorizontalHeaderLabels({ "Name", "Date", "Size" });
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table_->verticalHeader()->setVisible(false);
  table_->setSortingEnabled(true);
  table_->sortByColumn(kColName, Qt::AscendingOrder);
  table_->horizontalHeader()->setStretchLastSection(false);
  table_->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::Stretch);
  table_->horizontalHeader()->setSectionResizeMode(kColDate, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setSectionResizeMode(kColSize, QHeaderView::ResizeToContents);
  table_->horizontalHeader()->setHighlightSections(false);
  connect(table_, &QTableWidget::cellClicked, this, &SequencePanel::onCellClicked);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(header_);
  layout->addLayout(filter_row);
  layout->addWidget(picker_);
  layout->addWidget(table_);
  setLayout(layout);
}

QString SequencePanel::formatDate(int64_t ts_ns)
{
  if (ts_ns <= 0)
  {
    return QStringLiteral("--/--/----");
  }
  auto dt = QDateTime::fromMSecsSinceEpoch(ts_ns / 1'000'000LL, QTimeZone::utc());
  return dt.toString("dd/MM/yyyy");
}

void SequencePanel::populateSequences(const std::vector<SequenceInfo>& sequences)
{
  all_sequences_ = sequences;

  // Update the picker's "from" placeholder with the earliest available date
  // across all sequences. Skip entries with unknown timestamps (== 0).
  int64_t earliest = 0;
  for (const auto& seq : sequences)
  {
    if (seq.min_ts_ns > 0 && (earliest == 0 || seq.min_ts_ns < earliest))
    {
      earliest = seq.min_ts_ns;
    }
  }
  QDate earliest_date;
  if (earliest > 0)
  {
    earliest_date = QDateTime::fromMSecsSinceEpoch(earliest / 1'000'000LL).date();
  }
  picker_->setEarliestDate(earliest_date);

  // Disable sorting while populating to avoid repeated re-sorts.
  table_->setSortingEnabled(false);
  table_->setRowCount(0);

  if (sequences.empty())
  {
    header_->setText("Sequences (none found)");
    table_->setSortingEnabled(true);
    return;
  }

  header_->setText(QString("Sequences (%1)").arg(sequences.size()));
  table_->setRowCount(static_cast<int>(sequences.size()));

  for (int i = 0; i < static_cast<int>(sequences.size()); ++i)
  {
    const auto& seq = sequences[static_cast<size_t>(i)];

    // Name column — plain QTableWidgetItem (sorts alphabetically).
    auto* name_item = new QTableWidgetItem(QString::fromStdString(seq.name));
    table_->setItem(i, kColName, name_item);

    // Date column — NumericTableItem (sorts by raw nanoseconds).
    auto* date_item = new NumericTableItem(formatDate(seq.max_ts_ns));
    date_item->setData(Qt::UserRole, static_cast<qlonglong>(seq.max_ts_ns));
    table_->setItem(i, kColDate, date_item);

    // Size column — NumericTableItem (sorts by raw bytes).
    auto* size_item = new NumericTableItem(formatBytes(seq.total_size_bytes));
    size_item->setData(Qt::UserRole, static_cast<qlonglong>(seq.total_size_bytes));
    size_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    table_->setItem(i, kColSize, size_item);
  }

  table_->setSortingEnabled(true);
  applyFilter();
}

void SequencePanel::setLoading(bool loading)
{
  header_->setText(loading ? "Sequences (loading\u2026)" : "Sequences");
  table_->setEnabled(!loading);
}

void SequencePanel::onCellClicked(int row, int /*column*/)
{
  auto* item = table_->item(row, kColName);
  if (item)
  {
    emit sequenceSelected(item->text());
  }
}

void SequencePanel::onFilterChanged(const RangeFilter& filter)
{
  range_filter_ = filter;
  applyFilter();
}

// Returns true if the sequence [min_ts, max_ts] passes the date/time filter.
// See RangeFilter documentation in sequence_picker_widget.h for the four cases.
static bool matchesDateTimeFilter(int64_t min_ts, int64_t max_ts, const RangeFilter& f)
{
  // No filter set at all -> pass.
  if (!f.date_from && !f.date_to && !f.every_day)
  {
    return true;
  }

  // Unknown timestamp data -> don't hide.
  if (min_ts == 0 && max_ts == 0)
  {
    return true;
  }

  // Case "dates set, every_day off": single continuous interval.
  if (!f.every_day)
  {
    int64_t from_ns = std::numeric_limits<int64_t>::min();
    int64_t to_ns = std::numeric_limits<int64_t>::max();
    if (f.date_from)
    {
      QDateTime dt(*f.date_from, f.from_time, QTimeZone::utc());
      from_ns = dt.toMSecsSinceEpoch() * 1'000'000LL;
    }
    if (f.date_to)
    {
      // to_time is end-of-minute (see SequencePickerWidget::buildFilter),
      // so + 1ms makes it exclusive of the next minute.
      QDateTime dt(*f.date_to, f.to_time, QTimeZone::utc());
      to_ns = dt.toMSecsSinceEpoch() * 1'000'000LL + 1'000'000LL;
    }
    return !(max_ts < from_ns || min_ts > to_ns);
  }

  // every_day on: the filter is the time-of-day window [from_time, to_time]
  // applied to every day in the date range (or every day the sequence touches
  // if no date range).  The sequence matches if its time-of-day slice on at
  // least one of those days intersects the window.
  QDateTime min_dt = QDateTime::fromMSecsSinceEpoch(min_ts / 1'000'000LL, QTimeZone::utc());
  QDateTime max_dt = QDateTime::fromMSecsSinceEpoch(max_ts / 1'000'000LL, QTimeZone::utc());
  const QDate min_date = min_dt.date();
  const QDate max_date = max_dt.date();

  QDate from_day = f.date_from.value_or(min_date);
  QDate to_day = f.date_to.value_or(max_date);
  if (from_day < min_date)
  {
    from_day = min_date;
  }
  if (to_day > max_date)
  {
    to_day = max_date;
  }
  if (from_day > to_day)
  {
    return false;  // date range doesn't overlap the sequence at all
  }

  // On a middle day (neither sequence start nor end), the slice is the full
  // day, which always overlaps any non-empty time-of-day window — so the loop
  // terminates in O(1) for any sequence spanning >= 3 days in the checked range.
  for (QDate d = from_day; d <= to_day; d = d.addDays(1))
  {
    const QTime day_start = (d == min_date) ? min_dt.time() : QTime(0, 0);
    const QTime day_end = (d == max_date) ? max_dt.time() : QTime(23, 59, 59, 999);
    if (!(day_end < f.from_time || day_start > f.to_time))
    {
      return true;
    }
  }
  return false;
}

void SequencePanel::applyFilter()
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

  int visible = 0;
  for (int row = 0; row < table_->rowCount(); ++row)
  {
    auto* name_item = table_->item(row, kColName);
    if (!name_item)
    {
      continue;
    }
    const QString name = name_item->text();

    const bool name_matches =
        use_regex ? re.match(name).hasMatch() : name.contains(pattern, Qt::CaseInsensitive);

    // Date/time filter: look up min_ts from all_sequences_ by name (rows may
    // be sorted differently), max_ts from the date cell's UserRole.
    int64_t min_ts = 0;
    int64_t max_ts = 0;
    for (const auto& seq : all_sequences_)
    {
      if (QString::fromStdString(seq.name) == name)
      {
        min_ts = seq.min_ts_ns;
        break;
      }
    }
    if (auto* date_item = table_->item(row, kColDate))
    {
      max_ts = date_item->data(Qt::UserRole).toLongLong();
    }
    const bool date_matches = matchesDateTimeFilter(min_ts, max_ts, range_filter_);

    // Visible-set filter (driven by Lua query engine in plugin wrapper).
    bool metadata_matches = true;
    if (visible_sequences_.has_value())
    {
      metadata_matches = visible_sequences_->count(name.toStdString()) > 0;
    }

    const bool show = name_matches && date_matches && metadata_matches;
    table_->setRowHidden(row, !show);
    if (show)
    {
      ++visible;
    }
  }

  header_->setText(QString("Sequences (%1/%2)").arg(visible).arg(table_->rowCount()));
}

void SequencePanel::setVisibleSequences(const std::set<std::string>& visible_names)
{
  visible_sequences_ = visible_names;
  applyFilter();
}

void SequencePanel::clearVisibleSequences()
{
  visible_sequences_ = std::nullopt;
  applyFilter();
}

void SequencePanel::contextMenuEvent(QContextMenuEvent* event)
{
  auto* item = table_->itemAt(table_->viewport()->mapFromGlobal(event->globalPos()));
  if (!item)
  {
    QWidget::contextMenuEvent(event);
    return;
  }
  auto* name_item = table_->item(item->row(), kColName);
  if (!name_item)
  {
    return;
  }
  QString name = name_item->text();

  QMenu menu(this);
  auto* copy = menu.addAction("Copy name");
  connect(copy, &QAction::triggered, this, [name]() { QApplication::clipboard()->setText(name); });

  menu.exec(event->globalPos());
}
