/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QWidget>
#include <QDate>

class CalendarWidget : public QWidget
{
  Q_OBJECT

public:
  explicit CalendarWidget(QWidget* parent = nullptr);

  void setMonth(int year, int month);
  int year() const
  {
    return year_;
  }
  int month() const
  {
    return month_;
  }

  void setMediated(bool mediated);
  bool isMediated() const
  {
    return mediated_;
  }

  void setRange(const QDate& from, const QDate& to);
  void setHoverDate(const QDate& date);
  void clearRange();

  QDate rangeFrom() const
  {
    return range_from_;
  }
  QDate rangeTo() const
  {
    return range_to_;
  }

signals:
  void dateClicked(const QDate& date);
  void dateDoubleClicked(const QDate& date);
  void dateHovered(const QDate& date);
  void hoverLeft();

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void leaveEvent(QEvent* event) override;

private:
  QDate dateAtPosition(const QPoint& pos) const;
  QRect cellRect(int row, int col) const;
  int headerHeight() const;
  int firstDayColumn() const;

  // Which month to display
  int year_;
  int month_;

  // Range state
  QDate range_from_;
  QDate range_to_;
  QDate hover_date_;

  // Standalone state machine (used when not mediated)
  enum class State
  {
    Idle,
    Selecting,
    Committed
  };
  State state_ = State::Idle;

  bool mediated_ = false;
};
