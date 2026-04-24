#include "sequence_panel.h"

#include "drop_overlay.h"
#include "../format_utils.h"
#include "../picker/sequence_picker_widget.h"
#include "../colors.h"
#include "../upload/upload_utils.h"

#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
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

  setAcceptDrops(true);
  overlay_ = new DropOverlay(this);

  connect(this, &SequencePanel::dismissRequested, this,
          &SequencePanel::onDismiss);
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
  // Re-rendering wipes every row — our pending item pointers are stale.
  // We'll re-add pending rows below so in-flight uploads stay visible.
  QStringList pending_names = pending_rows_.keys();
  QSet<QString> still_failed = failed_rows_;
  pending_rows_.clear();
  failed_rows_.clear();

  if (sequences.empty() && pending_names.isEmpty())
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

  // Re-add any pending upload rows the refresh would have wiped.
  for (const auto& name : pending_names)
  {
    // Skip if the real sequence is now in the list (upload just succeeded).
    bool already_real = false;
    for (const auto& seq : sequences)
    {
      if (QString::fromStdString(seq.name) == name)
      {
        already_real = true;
        break;
      }
    }
    if (already_real) continue;

    int row = table_->rowCount();
    table_->insertRow(row);
    auto* name_item = new QTableWidgetItem(name);
    table_->setItem(row, kColName, name_item);
    auto* date_item = new QTableWidgetItem(
        still_failed.contains(name) ? "Failed" : "Uploading...");
    table_->setItem(row, kColDate, date_item);
    auto* size_item = new QTableWidgetItem(
        still_failed.contains(name) ? "Failed" : "…");
    if (still_failed.contains(name))
      size_item->setForeground(QBrush(kErrorRed));
    table_->setItem(row, kColSize, size_item);
    pending_rows_.insert(name, name_item);
    if (still_failed.contains(name)) failed_rows_.insert(name);
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

void SequencePanel::setPermissions(bool can_write, bool can_delete)
{
  can_write_ = can_write;
  can_delete_ = can_delete;
}

void SequencePanel::setConnected(bool connected)
{
  is_connected_ = connected;
}

void SequencePanel::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  if (overlay_) overlay_->setGeometry(rect());
}

void SequencePanel::dragEnterEvent(QDragEnterEvent* event)
{
  if (!event->mimeData()->hasUrls())
  {
    event->ignore();
    return;
  }

  using Mode = DropOverlay::Mode;
  if (!is_connected_)
    overlay_->setMode(Mode::NotConnected);
  else if (!can_write_)
    overlay_->setMode(Mode::NoWritePermission);
  else
    overlay_->setMode(Mode::Ready);

  if (is_connected_ && can_write_)
    event->acceptProposedAction();
  else
    event->ignore();
}

void SequencePanel::dragLeaveEvent(QDragLeaveEvent* event)
{
  overlay_->setMode(DropOverlay::Mode::Hidden);
  QWidget::dragLeaveEvent(event);
}

void SequencePanel::dropEvent(QDropEvent* event)
{
  overlay_->setMode(DropOverlay::Mode::Hidden);
  if (!is_connected_ || !can_write_)
  {
    event->ignore();
    return;
  }

  QStringList accepted;
  for (const auto& url : event->mimeData()->urls())
  {
    if (!url.isLocalFile()) continue;
    auto path = url.toLocalFile();
    if (acceptsMcapFile(path)) accepted << path;
  }

  if (!accepted.isEmpty())
  {
    emit filesDropped(accepted);
    event->acceptProposedAction();
  }
  else
  {
    event->ignore();
  }
}

QStringList SequencePanel::existingSequenceNames() const
{
  QStringList names;
  for (const auto& seq : all_sequences_)
    names << QString::fromStdString(seq.name);
  for (auto it = pending_rows_.begin(); it != pending_rows_.end(); ++it)
    names << it.key();
  return names;
}

void SequencePanel::onUploadStarted(const QString& sequence_name)
{
  table_->setSortingEnabled(false);
  int row = table_->rowCount();
  table_->insertRow(row);

  auto* name_item = new QTableWidgetItem(sequence_name);
  name_item->setFlags(name_item->flags() & ~Qt::ItemIsEditable);
  table_->setItem(row, kColName, name_item);

  auto* date_item = new QTableWidgetItem("Uploading...");
  date_item->setFlags(date_item->flags() & ~Qt::ItemIsEditable);
  table_->setItem(row, kColDate, date_item);

  auto* size_item = new QTableWidgetItem("0%");
  size_item->setFlags(size_item->flags() & ~Qt::ItemIsEditable);
  table_->setItem(row, kColSize, size_item);

  pending_rows_.insert(sequence_name, name_item);
  table_->setSortingEnabled(true);
}

void SequencePanel::onUploadProgress(const QString& sequence_name,
                                      int percent)
{
  auto it = pending_rows_.find(sequence_name);
  if (it == pending_rows_.end()) return;
  int row = table_->row(it.value());
  if (row < 0) return;
  auto* item = table_->item(row, kColSize);
  if (item) item->setText(QString("%1%").arg(percent));
}

void SequencePanel::onUploadSucceeded(const QString& sequence_name)
{
  auto it = pending_rows_.find(sequence_name);
  if (it == pending_rows_.end()) return;
  int row = table_->row(it.value());
  if (row >= 0) table_->removeRow(row);
  pending_rows_.erase(it);
}

void SequencePanel::onUploadFailed(const QString& sequence_name,
                                    const QString& message)
{
  auto it = pending_rows_.find(sequence_name);
  if (it == pending_rows_.end()) return;
  int row = table_->row(it.value());
  if (row < 0) return;
  auto* date_item = table_->item(row, kColDate);
  if (date_item) date_item->setText("Failed");
  auto* size_item = table_->item(row, kColSize);
  if (size_item)
  {
    size_item->setText(message.isEmpty() ? "Failed" : message);
    size_item->setForeground(QBrush(kErrorRed));
  }
  failed_rows_.insert(sequence_name);
}

void SequencePanel::keyPressEvent(QKeyEvent* event)
{
  if (event->key() != Qt::Key_Delete || !can_delete_)
  {
    QWidget::keyPressEvent(event);
    return;
  }

  int row = table_->currentRow();
  auto* name_item = row >= 0 ? table_->item(row, kColName) : nullptr;
  if (!name_item)
  {
    QWidget::keyPressEvent(event);
    return;
  }

  QString name = name_item->text();
  // In-flight uploads cannot be deleted (the row is not a server sequence yet).
  if (pending_rows_.contains(name) && !failed_rows_.contains(name))
    return;

  auto choice = QMessageBox::question(
      this, "Confirm Delete",
      QString("Delete sequence \"%1\"?").arg(name),
      QMessageBox::Yes | QMessageBox::No);
  if (choice == QMessageBox::Yes)
    emit deleteRequested({name});
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
  if (!name_item) return;
  QString name = name_item->text();
  bool is_pending = pending_rows_.contains(name);
  bool is_failed = failed_rows_.contains(name);

  QMenu menu(this);

  if (is_failed)
  {
    auto* dismiss = menu.addAction("Dismiss");
    connect(dismiss, &QAction::triggered, this,
            [this, name]() { emit dismissRequested(name); });
  }
  else if (!is_pending)
  {
    auto* del = menu.addAction("Delete");
    del->setEnabled(can_delete_);
    if (!can_delete_) del->setToolTip("Requires Delete permission");
    connect(del, &QAction::triggered, this, [this, name]() {
      auto choice = QMessageBox::question(
          this, "Confirm Delete",
          QString("Delete sequence \"%1\"?").arg(name),
          QMessageBox::Yes | QMessageBox::No);
      if (choice == QMessageBox::Yes)
        emit deleteRequested({name});
    });
  }

  auto* copy = menu.addAction("Copy name");
  connect(copy, &QAction::triggered, this, [name]() {
    QApplication::clipboard()->setText(name);
  });

  menu.exec(event->globalPos());
}

void SequencePanel::onDismiss(const QString& sequence_name)
{
  auto it = pending_rows_.find(sequence_name);
  if (it == pending_rows_.end()) return;
  int row = table_->row(it.value());
  if (row >= 0) table_->removeRow(row);
  pending_rows_.erase(it);
  failed_rows_.remove(sequence_name);
}
