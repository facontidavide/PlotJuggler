#pragma once

#include <QWidget>
#include <QDate>
#include <QList>

class QPushButton;

class CalendarWidget;

class DualCalendarWidget : public QWidget
{
  Q_OBJECT

public:
  explicit DualCalendarWidget(QWidget* parent = nullptr);

  // Navigate to show `from` (and `to`) and set the selection highlights
  // without emitting signals. Either date may be invalid (e.g., while the
  // user is mid-typing). If both invalid, range is cleared but the
  // currently-shown months are preserved.
  void setExternalRange(const QDate& from, const QDate& to);

signals:
  void rangeCommitted(const QDate& from, const QDate& to);
  // Emitted during selection as the user hovers over dates.
  // from/to reflect the tentative range (anchor + hover).
  void rangePreview(const QDate& from, const QDate& to);

private slots:
  void onDateClicked(const QDate& date);
  void onDateHovered(const QDate& date);
  void onHoverLeft();

  void leftPrev();
  void leftNext();
  void rightPrev();
  void rightNext();

private:
  void updateCalendars();
  void updateNavButtons();
  void broadcastState();

  static void advanceMonth(int& year, int& month, int delta);

  QList<CalendarWidget*> calendars_;

  QPushButton* left_prev_;
  QPushButton* left_next_;
  QPushButton* right_prev_;
  QPushButton* right_next_;

  int left_year_;
  int left_month_;
  int left_max_year_;
  int left_max_month_;
  int right_year_;
  int right_month_;
  int right_min_year_;
  int right_min_month_;

  // Range state: Idle -> click -> Selecting (anchor set) -> click -> Idle (range committed)
  bool selecting_ = false;
  QDate range_from_;
  QDate range_to_;
  QDate hover_date_;
};
