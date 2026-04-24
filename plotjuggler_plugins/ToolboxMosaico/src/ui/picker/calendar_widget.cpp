/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "calendar_widget.h"
#include "../theme_utils.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

static constexpr int Rows = 6;
static constexpr int Cols = 7;
static constexpr int HeaderPad = 8;

CalendarWidget::CalendarWidget(QWidget* parent)
  : QWidget(parent), year_(QDate::currentDate().year()), month_(QDate::currentDate().month())
{
  setMouseTracking(true);
  setMinimumSize(320, 260);
}

void CalendarWidget::setMonth(int year, int month)
{
  year_ = year;
  month_ = month;
  update();
}

void CalendarWidget::setMediated(bool mediated)
{
  mediated_ = mediated;
}

void CalendarWidget::setRange(const QDate& from, const QDate& to)
{
  range_from_ = from;
  range_to_ = to;
  update();
}

void CalendarWidget::setHoverDate(const QDate& date)
{
  hover_date_ = date;
  update();
}

void CalendarWidget::clearRange()
{
  range_from_ = QDate();
  range_to_ = QDate();
  hover_date_ = QDate();
  state_ = State::Idle;
  update();
}

int CalendarWidget::headerHeight() const
{
  return fontMetrics().height() * 2 + HeaderPad * 3;
}

int CalendarWidget::firstDayColumn() const
{
  // Monday = 0, Tuesday = 1, ... Sunday = 6
  int dow = QDate(year_, month_, 1).dayOfWeek();  // Qt: 1=Mon, 7=Sun
  return dow - 1;
}

QRect CalendarWidget::cellRect(int row, int col) const
{
  int hh = headerHeight();
  int cellW = width() / Cols;
  int cellH = (height() - hh) / Rows;
  return QRect(col * cellW, hh + row * cellH, cellW, cellH);
}

QDate CalendarWidget::dateAtPosition(const QPoint& pos) const
{
  int hh = headerHeight();
  if (pos.y() < hh)
  {
    return QDate();
  }

  int cellW = width() / Cols;
  int cellH = (height() - hh) / Rows;
  int col = pos.x() / cellW;
  int row = (pos.y() - hh) / cellH;

  if (col < 0 || col >= Cols || row < 0 || row >= Rows)
  {
    return QDate();
  }

  int dayIndex = row * Cols + col - firstDayColumn();
  int day = dayIndex + 1;

  if (day < 1 || day > QDate(year_, month_, 1).daysInMonth())
  {
    return QDate();
  }

  return QDate(year_, month_, day);
}

void CalendarWidget::paintEvent(QPaintEvent* /*event*/)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  const QPalette& pal = palette();
  // QWidget background is transparent via global QSS; use the themed widget_background.
  bool dark = isDarkTheme();
  p.fillRect(rect(), dark ? QColor(0x33, 0x33, 0x33) : QColor(0xff, 0xff, 0xff));

  int hh = headerHeight();
  int cellW = width() / Cols;
  int fmH = fontMetrics().height();

  // --- Month/Year title ---
  QFont titleFont = font();
  titleFont.setBold(true);
  titleFont.setPointSize(font().pointSize() + 2);
  p.setFont(titleFont);

  QString title = QDate(year_, month_, 1).toString("MMMM yyyy");
  p.setPen(pal.color(QPalette::WindowText));
  p.drawText(QRect(0, 0, width(), fmH + HeaderPad * 2), Qt::AlignCenter, title);

  // --- Day name headers ---
  p.setFont(font());
  static const char* dayNames[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  int dayHeaderY = fmH + HeaderPad * 2;
  QColor weekendColor(0xef, 0x53, 0x50);  // semantic red for weekends
  QColor headerColor = pal.color(QPalette::Mid);
  for (int c = 0; c < Cols; ++c)
  {
    QRect r(c * cellW, dayHeaderY, cellW, fmH + HeaderPad);
    p.setPen((c >= 5) ? weekendColor : headerColor);
    p.drawText(r, Qt::AlignCenter, dayNames[c]);
  }

  // --- Compute effective range for painting ---
  QDate effFrom = range_from_;
  QDate effTo = range_to_;

  // In selecting state, use hoverDate as the tentative endpoint
  if (effFrom.isValid() && !effTo.isValid() && hover_date_.isValid())
  {
    effTo = hover_date_;
  }

  // Normalize so effFrom <= effTo
  if (effFrom.isValid() && effTo.isValid() && effFrom > effTo)
  {
    std::swap(effFrom, effTo);
  }

  // --- Day cells ---
  int daysInMonth = QDate(year_, month_, 1).daysInMonth();
  int firstCol = firstDayColumn();

  QColor fromColor(0x2e, 0xcc, 0x71);  // semantic green for "from"
  QColor toColor(0xe7, 0x4c, 0x3c);    // semantic red for "to"
  QColor rangeColor = pal.color(QPalette::Highlight);
  rangeColor.setAlpha(60);
  QColor hoverBrush = pal.color(QPalette::Midlight);
  QColor hoverBorder = pal.color(QPalette::Mid);

  for (int day = 1; day <= daysInMonth; ++day)
  {
    int idx = (day - 1) + firstCol;
    int row = idx / Cols;
    int col = idx % Cols;
    QRect cell = cellRect(row, col);

    QDate date(year_, month_, day);

    // Determine cell state
    bool isFrom = effFrom.isValid() && date == effFrom;
    bool isTo = effTo.isValid() && date == effTo;
    bool inRange = effFrom.isValid() && effTo.isValid() && date > effFrom && date < effTo;
    bool isHovered = hover_date_.isValid() && date == hover_date_ && !isFrom && !isTo && !inRange;

    // --- Paint background ---
    QRect inner = cell.adjusted(1, 1, -1, -1);

    if (isFrom)
    {
      p.setBrush(fromColor);
      p.setPen(Qt::NoPen);
      p.drawRoundedRect(inner, 6, 6);
    }
    else if (isTo)
    {
      p.setBrush(toColor);
      p.setPen(Qt::NoPen);
      p.drawRoundedRect(inner, 6, 6);
    }
    else if (inRange)
    {
      p.setBrush(rangeColor);
      p.setPen(Qt::NoPen);
      p.drawRect(inner);
    }
    else if (isHovered)
    {
      p.setBrush(hoverBrush);
      p.setPen(QPen(hoverBorder, 1));
      p.drawRoundedRect(inner, 4, 4);
    }

    // --- Paint text ---
    QColor textColor;
    if (isFrom || isTo)
    {
      textColor = Qt::white;
    }
    else if (col >= 5)
    {
      textColor = weekendColor;
    }
    else
    {
      textColor = pal.color(QPalette::WindowText);
    }

    p.setPen(textColor);
    p.setFont(font());
    p.drawText(cell, Qt::AlignCenter, QString::number(day));
  }

  // --- Grid lines ---
  QColor gridColor = pal.color(QPalette::Mid);
  gridColor.setAlpha(80);
  p.setPen(QPen(gridColor, 0.5));
  for (int r = 0; r <= Rows; ++r)
  {
    QRect cell = cellRect(r, 0);
    p.drawLine(0, cell.y(), width(), cell.y());
  }
  for (int c = 0; c <= Cols; ++c)
  {
    int x = c * cellW;
    p.drawLine(x, hh, x, height());
  }
}

void CalendarWidget::mouseMoveEvent(QMouseEvent* event)
{
  QDate date = dateAtPosition(event->pos());
  setCursor(date.isValid() ? Qt::PointingHandCursor : Qt::ArrowCursor);

  if (date.isValid())
  {
    if (mediated_)
    {
      emit dateHovered(date);
    }
    else
    {
      if (state_ == State::Selecting)
      {
        hover_date_ = date;
        update();
      }
      else if (hover_date_ != date)
      {
        hover_date_ = date;
        update();
      }
    }
  }
  else
  {
    if (mediated_)
    {
      emit hoverLeft();
    }
    else if (hover_date_.isValid())
    {
      hover_date_ = QDate();
      update();
    }
  }
}

void CalendarWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    return;
  }

  QDate date = dateAtPosition(event->pos());
  if (!date.isValid())
  {
    return;
  }

  if (mediated_)
  {
    emit dateClicked(date);
    return;
  }

  // Standalone state machine
  switch (state_)
  {
    case State::Idle:
      range_from_ = date;
      range_to_ = QDate();
      state_ = State::Selecting;
      break;
    case State::Selecting:
      range_to_ = date;
      // Normalize
      if (range_from_ > range_to_)
      {
        std::swap(range_from_, range_to_);
      }
      hover_date_ = QDate();
      state_ = State::Committed;
      break;
    case State::Committed:
      if (qAbs(range_from_.daysTo(date)) <= qAbs(range_to_.daysTo(date)))
      {
        range_from_ = date;
      }
      else
      {
        range_to_ = date;
      }
      if (range_from_ > range_to_)
      {
        std::swap(range_from_, range_to_);
      }
      break;
  }
  update();
}

void CalendarWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    return;
  }

  QDate date = dateAtPosition(event->pos());
  if (!date.isValid())
  {
    return;
  }

  if (mediated_)
  {
    emit dateDoubleClicked(date);
    return;
  }

  // Standalone: reset range to single day
  range_from_ = date;
  range_to_ = date;
  hover_date_ = QDate();
  state_ = State::Committed;
  update();
}

void CalendarWidget::leaveEvent(QEvent* event)
{
  Q_UNUSED(event);

  if (mediated_)
  {
    emit hoverLeft();
  }
  else if (hover_date_.isValid())
  {
    hover_date_ = QDate();
    update();
  }
}
