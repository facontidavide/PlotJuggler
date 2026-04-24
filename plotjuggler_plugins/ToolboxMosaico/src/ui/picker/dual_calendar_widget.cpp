/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "dual_calendar_widget.h"
#include "../theme_utils.h"
#include "calendar_widget.h"

#include <PlotJuggler/svg_util.h>

#include <QDate>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
// Recolor an SVG-derived pixmap (alpha mask + grayscale fill) to a solid
// `color` using the source's alpha as a stencil.
QIcon tintedIcon(const QPixmap& src, const QColor& color)
{
  // updateNavButtons() runs on every hover event (once per mouse-move over
  // a new date cell) and calls this up to 4x. The input space is tiny —
  // 2 source pixmaps × a handful of tint colors — so the composite work
  // is a no-op on 99% of calls. Cache keyed on (pixmap identity, color).
  // QPixmap::cacheKey() is stable across calls for the same underlying data,
  // so theme swaps (which produce new pixmaps) automatically get new entries.
  using Key = QPair<qint64, QRgb>;
  static QHash<Key, QIcon> cache;
  const Key key{ src.cacheKey(), color.rgba() };
  auto it = cache.find(key);
  if (it != cache.end())
  {
    return it.value();
  }

  QPixmap result(src.size());
  result.fill(Qt::transparent);
  QPainter p(&result);
  p.drawPixmap(0, 0, src);
  p.setCompositionMode(QPainter::CompositionMode_SourceIn);
  p.fillRect(src.rect(), color);
  p.end();

  QIcon icon(result);
  cache.insert(key, icon);
  return icon;
}

// Signed month comparison. Returns 0 for an invalid date so "no range set"
// behaves as "no direction hint".
int monthCmp(const QDate& d, int year, int month)
{
  if (!d.isValid())
  {
    return 0;
  }
  if (d.year() != year)
  {
    return (d.year() < year) ? -1 : +1;
  }
  if (d.month() != month)
  {
    return (d.month() < month) ? -1 : +1;
  }
  return 0;
}

// Direction-hint colors, matching the from/to cell highlights in
// calendar_widget.cpp so the nav arrows semantically mirror the cells.
const QColor kFromHintColor(0x2e, 0xcc, 0x71);  // green
const QColor kToHintColor(0xe7, 0x4c, 0x3c);    // red
}  // namespace

void DualCalendarWidget::advanceMonth(int& year, int& month, int delta)
{
  month += delta;
  while (month > 12)
  {
    month -= 12;
    year++;
  }
  while (month < 1)
  {
    month += 12;
    year--;
  }
}

DualCalendarWidget::DualCalendarWidget(QWidget* parent)
  : QWidget(parent)
  , left_prev_(new QPushButton)
  , left_next_(new QPushButton)
  , right_prev_(new QPushButton)
  , right_next_(new QPushButton)
  , left_year_(QDate::currentDate().year())
  , left_month_(QDate::currentDate().month())
  , left_max_year_(QDate::currentDate().year())
  , left_max_month_(QDate::currentDate().month())
{
  // Right defaults to left + 1, and can never go below this
  right_year_ = left_year_;
  right_month_ = left_month_;
  advanceMonth(right_year_, right_month_, 1);
  right_min_year_ = right_year_;
  right_min_month_ = right_month_;

  // --- Icons + fixed size on all nav buttons ---
  const QSize btnSize(32, 24);
  const QSize iconSize(14, 14);
  const auto theme = themeName();
  const QIcon left_icon(LoadSvg(":/resources/svg/left-arrow.svg", theme));
  const QIcon right_icon(LoadSvg(":/resources/svg/right-arrow.svg", theme));
  for (auto* b : { left_prev_, right_prev_ })
  {
    b->setIcon(left_icon);
    b->setIconSize(iconSize);
    b->setFixedSize(btnSize);
  }
  for (auto* b : { left_next_, right_next_ })
  {
    b->setIcon(right_icon);
    b->setIconSize(iconSize);
    b->setFixedSize(btnSize);
  }

  // --- Left calendar with its own nav ---
  auto* leftNav = new QHBoxLayout;
  leftNav->addWidget(left_prev_);
  leftNav->addStretch();
  leftNav->addWidget(left_next_);

  auto* leftCol = new QVBoxLayout;
  leftCol->addLayout(leftNav);
  auto* leftCal = new CalendarWidget;
  leftCal->setMediated(true);
  calendars_.append(leftCal);
  leftCol->addWidget(leftCal);

  connect(left_prev_, &QPushButton::clicked, this, &DualCalendarWidget::leftPrev);
  connect(left_next_, &QPushButton::clicked, this, &DualCalendarWidget::leftNext);

  // --- Right calendar with its own nav ---
  auto* rightNav = new QHBoxLayout;
  rightNav->addWidget(right_prev_);
  rightNav->addStretch();
  rightNav->addWidget(right_next_);

  auto* rightCol = new QVBoxLayout;
  rightCol->addLayout(rightNav);
  auto* rightCal = new CalendarWidget;
  rightCal->setMediated(true);
  calendars_.append(rightCal);
  rightCol->addWidget(rightCal);

  connect(right_prev_, &QPushButton::clicked, this, &DualCalendarWidget::rightPrev);
  connect(right_next_, &QPushButton::clicked, this, &DualCalendarWidget::rightNext);

  // --- Wire up calendar signals ---
  for (auto* cal : calendars_)
  {
    connect(cal, &CalendarWidget::dateClicked, this, &DualCalendarWidget::onDateClicked);
    connect(cal, &CalendarWidget::dateHovered, this, &DualCalendarWidget::onDateHovered);
    connect(cal, &CalendarWidget::hoverLeft, this, &DualCalendarWidget::onHoverLeft);
  }

  // --- Main layout ---
  auto* mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(8, 8, 8, 8);
  mainLayout->setSpacing(16);
  mainLayout->addLayout(leftCol);
  mainLayout->addLayout(rightCol);

  updateCalendars();
}

void DualCalendarWidget::setExternalRange(const QDate& from, const QDate& to)
{
  // Refresh today's bounds — cheap, and covers widgets that have lived past
  // midnight. left_max_ is "today's month"; right_min_ is "today + 1 month".
  const QDate today = QDate::currentDate();
  left_max_year_ = today.year();
  left_max_month_ = today.month();
  right_min_year_ = left_max_year_;
  right_min_month_ = left_max_month_;
  advanceMonth(right_min_year_, right_min_month_, 1);

  // Decide target months.
  //   from valid, to strictly later month -> left = from.month, right = to.month
  //   from valid, to same month or invalid -> left = from.month; KEEP right
  //       (forcing right=left+1 causes jarring jumps when the user collapses
  //       a range to a single month)
  //   from invalid, to valid              -> right = to.month, left = right-1
  //   both invalid                        -> keep both (just clear the range)
  int new_left_year = left_year_;
  int new_left_month = left_month_;
  int new_right_year = right_year_;
  int new_right_month = right_month_;

  if (from.isValid())
  {
    new_left_year = from.year();
    new_left_month = from.month();

    const bool to_strictly_later =
        to.isValid() &&
        (to.year() > new_left_year || (to.year() == new_left_year && to.month() > new_left_month));
    if (to_strictly_later)
    {
      new_right_year = to.year();
      new_right_month = to.month();
    }
  }
  else if (to.isValid())
  {
    new_right_year = to.year();
    new_right_month = to.month();
    new_left_year = new_right_year;
    new_left_month = new_right_month;
    advanceMonth(new_left_year, new_left_month, -1);
  }

  // Clamp: left can't exceed today; right must lead left by at least one month.
  // We deliberately do NOT clamp right up to right_min_ — a past-only range
  // legitimately puts right in the past, and forcing it up to right_min would
  // leave a years-wide visual gap and strand the nav (see rightPrev / the
  // atMinimum check below, which relax when right is already below right_min_).
  auto monthBefore = [](int y1, int m1, int y2, int m2) {
    return y1 < y2 || (y1 == y2 && m1 < m2);
  };
  if (monthBefore(left_max_year_, left_max_month_, new_left_year, new_left_month))
  {
    new_left_year = left_max_year_;
    new_left_month = left_max_month_;
  }
  int left_plus_one_y = new_left_year;
  int left_plus_one_m = new_left_month;
  advanceMonth(left_plus_one_y, left_plus_one_m, 1);
  if (monthBefore(new_right_year, new_right_month, left_plus_one_y, left_plus_one_m))
  {
    new_right_year = left_plus_one_y;
    new_right_month = left_plus_one_m;
  }

  QDate new_from = from.isValid() ? from : QDate();
  QDate new_to = to.isValid() ? to : QDate();

  // Early out when nothing would change.
  if (new_left_year == left_year_ && new_left_month == left_month_ &&
      new_right_year == right_year_ && new_right_month == right_month_ && new_from == range_from_ &&
      new_to == range_to_ && !selecting_)
  {
    return;
  }

  left_year_ = new_left_year;
  left_month_ = new_left_month;
  right_year_ = new_right_year;
  right_month_ = new_right_month;
  range_from_ = new_from;
  range_to_ = new_to;
  hover_date_ = QDate();
  selecting_ = false;

  updateCalendars();
}

void DualCalendarWidget::leftPrev()
{
  advanceMonth(left_year_, left_month_, -1);
  updateCalendars();
}

void DualCalendarWidget::leftNext()
{
  int candYear = left_year_, candMonth = left_month_;
  advanceMonth(candYear, candMonth, 1);
  // Can't go above the maximum (today's month)
  if (candYear > left_max_year_ || (candYear == left_max_year_ && candMonth > left_max_month_))
  {
    return;
  }
  // Can't go past one month before right
  if (candYear > right_year_ || (candYear == right_year_ && candMonth >= right_month_))
  {
    return;
  }
  left_year_ = candYear;
  left_month_ = candMonth;
  updateCalendars();
}

void DualCalendarWidget::rightPrev()
{
  int candYear = right_year_, candMonth = right_month_;
  advanceMonth(candYear, candMonth, -1);
  // Can't merge with left.
  if (candYear < left_year_ || (candYear == left_year_ && candMonth <= left_month_))
  {
    return;
  }
  // right_min acts as a "one-way gate" toward the past: if we're already
  // above it, stepping back across the gate is blocked; if we're already
  // below it (external past range), we're free to navigate further within
  // the past. The user can always recover via rightNext.
  const bool currently_ge_min = right_year_ > right_min_year_ || (right_year_ == right_min_year_ &&
                                                                  right_month_ >= right_min_month_);
  if (currently_ge_min &&
      (candYear < right_min_year_ || (candYear == right_min_year_ && candMonth < right_min_month_)))
  {
    return;
  }
  right_year_ = candYear;
  right_month_ = candMonth;
  updateCalendars();
}

void DualCalendarWidget::rightNext()
{
  advanceMonth(right_year_, right_month_, 1);
  updateCalendars();
}

void DualCalendarWidget::updateCalendars()
{
  calendars_[0]->setMonth(left_year_, left_month_);
  calendars_[1]->setMonth(right_year_, right_month_);

  updateNavButtons();
  broadcastState();
}

void DualCalendarWidget::updateNavButtons()
{
  // Left next: disabled if left+1 would exceed maximum or reach right
  int lnY = left_year_, lnM = left_month_;
  advanceMonth(lnY, lnM, 1);
  bool atMax = lnY > left_max_year_ || (lnY == left_max_year_ && lnM > left_max_month_);
  bool atRight = lnY > right_year_ || (lnY == right_year_ && lnM >= right_month_);
  left_next_->setEnabled(!atMax && !atRight);

  // Right prev: disabled if right-1 would merge with left. right_min is a
  // one-way gate — only enforced when right is currently at or above it.
  int rpY = right_year_, rpM = right_month_;
  advanceMonth(rpY, rpM, -1);
  const bool currently_ge_min = right_year_ > right_min_year_ || (right_year_ == right_min_year_ &&
                                                                  right_month_ >= right_min_month_);
  bool atMinimum = currently_ge_min &&
                   (rpY < right_min_year_ || (rpY == right_min_year_ && rpM < right_min_month_));
  bool atLeft = rpY < left_year_ || (rpY == left_year_ && rpM <= left_month_);
  right_prev_->setEnabled(!atMinimum && !atLeft);

  // Left prev and right next: always enabled
  left_prev_->setEnabled(true);
  right_next_->setEnabled(true);

  // --- Direction hint: tint the arrow green/red if clicking it reveals the
  // start/end date (currently outside both calendars' views).
  const auto theme = themeName();
  const QPixmap& left_src = LoadSvg(":/resources/svg/left-arrow.svg", theme);
  const QPixmap& right_src = LoadSvg(":/resources/svg/right-arrow.svg", theme);

  const int fromL = monthCmp(range_from_, left_year_, left_month_);
  const int fromR = monthCmp(range_from_, right_year_, right_month_);
  const int toL = monthCmp(range_to_, left_year_, left_month_);
  const int toR = monthCmp(range_to_, right_year_, right_month_);

  // "In the gap" = strictly after L AND strictly before R.
  const bool from_before_l = fromL < 0;
  const bool from_in_gap = fromL > 0 && fromR < 0;
  const bool from_after_r = fromR > 0;
  const bool to_before_l = toL < 0;
  const bool to_in_gap = toL > 0 && toR < 0;
  const bool to_after_r = toR > 0;

  auto applyHint = [](QPushButton* btn, const QPixmap& src, bool hint_from, bool hint_to) {
    if (hint_from)
    {
      btn->setIcon(tintedIcon(src, kFromHintColor));
    }
    else if (hint_to)
    {
      btn->setIcon(tintedIcon(src, kToHintColor));
    }
    else
    {
      btn->setIcon(QIcon(src));
    }
  };

  applyHint(left_prev_, left_src, from_before_l, to_before_l);
  applyHint(left_next_, right_src, from_in_gap, to_in_gap);
  applyHint(right_prev_, left_src, from_in_gap, to_in_gap);
  applyHint(right_next_, right_src, from_after_r, to_after_r);
}

void DualCalendarWidget::onDateClicked(const QDate& date)
{
  if (!selecting_)
  {
    // First click: anchor
    range_from_ = date;
    range_to_ = QDate();
    selecting_ = true;
  }
  else
  {
    // Second click: commit range
    range_to_ = date;
    if (range_from_ > range_to_)
    {
      std::swap(range_from_, range_to_);
    }
    hover_date_ = QDate();
    selecting_ = false;
    emit rangeCommitted(range_from_, range_to_);
  }
  broadcastState();
}

void DualCalendarWidget::onDateHovered(const QDate& date)
{
  hover_date_ = date;
  broadcastState();
}

void DualCalendarWidget::onHoverLeft()
{
  if (hover_date_.isValid())
  {
    hover_date_ = QDate();
    broadcastState();
  }
}

void DualCalendarWidget::broadcastState()
{
  QDate effFrom = range_from_;
  QDate effTo = range_to_;

  // While selecting, hover acts as tentative endpoint
  if (selecting_ && hover_date_.isValid())
  {
    effTo = hover_date_;
  }

  if (effFrom.isValid() && effTo.isValid() && effFrom > effTo)
  {
    std::swap(effFrom, effTo);
  }

  for (auto* cal : calendars_)
  {
    cal->setRange(effFrom, effTo);
    cal->setHoverDate(hover_date_);
  }

  updateNavButtons();

  if (selecting_ && effFrom.isValid() && effTo.isValid())
  {
    emit rangePreview(effFrom, effTo);
  }
}
