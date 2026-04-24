/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QDate>
#include <QTime>
#include <QWidget>

#include <optional>

class QButtonGroup;
class QLineEdit;
class QPushButton;

class DualCalendarWidget;
class TimePickerWidget;

// Filter for the sequence list.
//
// Semantics (the four combinations of date range and every_day):
//   (no dates,  every_day=off) -> no filter
//   (no dates,  every_day=on ) -> time-of-day [from_time, to_time] across all dates
//   (dates set, every_day=off) -> single continuous interval
//                                 [date_from + from_time, date_to + to_time]
//   (dates set, every_day=on ) -> time-of-day [from_time, to_time] on each day
//                                 in [date_from, date_to]
struct RangeFilter
{
  std::optional<QDate> date_from;
  std::optional<QDate> date_to;
  QTime from_time = QTime(0, 0);
  QTime to_time = QTime(23, 59, 59, 999);
  bool every_day = false;
};

// Foldable date/time range picker for filtering sequences.
//
// The inline part (presets + duration fields) lives in whatever layout the
// parent provides. The expandable parts (dual calendar + time picker) are an
// overlay widget parented to window(), positioned to the right of the
// calendar toggle button and floating over sibling panels.
//
// Emits filterChanged() on every user interaction — no "apply" button.
class SequencePickerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit SequencePickerWidget(QWidget* parent = nullptr);
  ~SequencePickerWidget() override;

  // Set the placeholder of the "from" field to the given date. Pass an
  // invalid QDate to reset to the generic "DD/MM/YYYY" hint.
  void setEarliestDate(const QDate& date);

signals:
  void filterChanged(const RangeFilter& filter);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void changeEvent(QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private slots:
  void onPresetClicked(int id);
  void checkCustomState();
  void toggleCalendar();
  void onCalendarRangeCommitted(const QDate& from, const QDate& to);
  void onCalendarRangePreview(const QDate& from, const QDate& to);
  void onTimeChanged();

private:
  void emitFilter();
  void repositionOverlay();
  void updateOverlayStyle();
  RangeFilter buildFilter() const;
  void applyPreset(int id);
  void updateFieldsFromPreset(const QDate& from, const QDate& to);
  int matchingPreset() const;
  // Parse the current text in from_edit_/to_edit_ and push the range to the
  // dual calendar so it navigates to the typed month and highlights the range.
  // Safe when dual_calendar_ is null (before the overlay is shown).
  void syncCalendarToFields();

  static constexpr int kPresetAll = 0;
  static constexpr int kPresetPast24h = 1;
  static constexpr int kPresetLast7Days = 2;
  static constexpr int kPresetLastMonth = 3;

  // Inline widgets (owned by this widget's layout)
  QButtonGroup* preset_group_ = nullptr;
  QPushButton* all_button_ = nullptr;
  QLineEdit* from_edit_ = nullptr;
  QLineEdit* to_edit_ = nullptr;
  QPushButton* calendar_button_ = nullptr;

  // Overlay widgets (parented to window())
  QWidget* overlay_ = nullptr;
  DualCalendarWidget* dual_calendar_ = nullptr;
  TimePickerWidget* time_picker_ = nullptr;

  bool calendar_visible_ = false;
};
