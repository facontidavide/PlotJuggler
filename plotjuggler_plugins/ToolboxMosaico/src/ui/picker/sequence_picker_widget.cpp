/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "sequence_picker_widget.h"

#include "../theme_utils.h"
#include "dual_calendar_widget.h"
#include "time_picker_widget.h"

#include <PlotJuggler/svg_util.h>

#include <QApplication>
#include <QButtonGroup>
#include <QDate>
#include <QDateTime>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QTimeZone>
#include <QTransform>
#include <QVBoxLayout>

namespace
{
// Cached icons for the calendar-toggle button: right-arrow when the overlay
// is collapsed, same icon rotated 90° (pointing down) when it is expanded.
// Re-fetched when the theme changes via SequencePickerWidget::changeEvent().
struct ChevronPair
{
  QIcon collapsed;  // right-arrow (points "into" the overlay to open it)
  QIcon expanded;   // rotated 90° clockwise — points down
};

ChevronPair makeChevronPair(const QString& theme)
{
  const QPixmap& right = LoadSvg(":/resources/svg/right-arrow.svg", theme);
  QTransform t;
  t.rotate(90);
  QPixmap down = right.transformed(t, Qt::SmoothTransformation);
  return { QIcon(right), QIcon(down) };
}
}  // namespace

SequencePickerWidget::SequencePickerWidget(QWidget* parent) : QWidget(parent)
{
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);

  // --- Row 1: Preset buttons ---
  auto* preset_row = new QHBoxLayout;

  preset_group_ = new QButtonGroup(this);
  preset_group_->setExclusive(true);

  auto add_preset = [&](int id, const QString& text) -> QPushButton* {
    auto* btn = new QPushButton(text);
    btn->setCheckable(true);
    btn->setCursor(Qt::PointingHandCursor);
    preset_group_->addButton(btn, id);
    preset_row->addWidget(btn);
    return btn;
  };

  all_button_ = add_preset(kPresetAll, "All");
  add_preset(kPresetPast24h, "Past 24h");
  add_preset(kPresetLast7Days, "Last 7 Days");
  add_preset(kPresetLastMonth, "Last Month");

  all_button_->setChecked(true);

  connect(preset_group_, &QButtonGroup::idClicked, this, &SequencePickerWidget::onPresetClicked);

  main_layout->addLayout(preset_row);

  // --- Row 2: Date inputs + calendar toggle ---
  auto* date_row = new QHBoxLayout;

  from_edit_ = new QLineEdit;
  from_edit_->setPlaceholderText("DD/MM/YYYY");

  auto* arrow_label = new QLabel(QString::fromUtf8("\u2192"));

  to_edit_ = new QLineEdit;
  // Placeholder is today's date — hints "up to now" when the user doesn't
  // type anything. Refreshed on each show in case the widget lives past
  // midnight.
  to_edit_->setPlaceholderText(QDate::currentDate().toString("dd/MM/yyyy"));

  calendar_button_ = new QPushButton;
  calendar_button_->setCursor(Qt::PointingHandCursor);
  calendar_button_->setFixedSize(32, 24);
  calendar_button_->setIconSize(QSize(14, 14));
  calendar_button_->setIcon(makeChevronPair(themeName()).collapsed);

  date_row->addWidget(from_edit_, 1);
  date_row->addWidget(arrow_label);
  date_row->addWidget(to_edit_, 1);
  date_row->addWidget(calendar_button_);

  connect(from_edit_, &QLineEdit::textChanged, this, &SequencePickerWidget::checkCustomState);
  connect(to_edit_, &QLineEdit::textChanged, this, &SequencePickerWidget::checkCustomState);
  connect(calendar_button_, &QPushButton::clicked, this, &SequencePickerWidget::toggleCalendar);

  main_layout->addLayout(date_row);
}

SequencePickerWidget::~SequencePickerWidget()
{
  // overlay_ is parented to window(), not to us — delete it explicitly.
  delete overlay_;
}

void SequencePickerWidget::setEarliestDate(const QDate& date)
{
  from_edit_->setPlaceholderText(date.isValid() ? date.toString("dd/MM/yyyy") :
                                                  QStringLiteral("DD/MM/YYYY"));
}

// --- Overlay creation (deferred until first show, so window() is valid) ---

void SequencePickerWidget::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);

  // Refresh the "to" placeholder so it tracks today's date across midnight
  // if the plugin is opened/closed repeatedly.
  to_edit_->setPlaceholderText(QDate::currentDate().toString("dd/MM/yyyy"));

  if (!overlay_ && window())
  {
    overlay_ = new QWidget(window());
    overlay_->setObjectName("PickerOverlay");
    updateOverlayStyle();

    auto* overlay_layout = new QVBoxLayout(overlay_);

    dual_calendar_ = new DualCalendarWidget;
    overlay_layout->addWidget(dual_calendar_);

    time_picker_ = new TimePickerWidget;
    overlay_layout->addWidget(time_picker_);

    overlay_->setVisible(false);

    connect(dual_calendar_, &DualCalendarWidget::rangeCommitted, this,
            &SequencePickerWidget::onCalendarRangeCommitted);
    connect(dual_calendar_, &DualCalendarWidget::rangePreview, this,
            &SequencePickerWidget::onCalendarRangePreview);
    connect(time_picker_, &TimePickerWidget::timeChanged, this,
            &SequencePickerWidget::onTimeChanged);

    // Install event filter on the window to reposition overlay on resize/move.
    window()->installEventFilter(this);
  }
}

void SequencePickerWidget::changeEvent(QEvent* event)
{
  if (event->type() == QEvent::StyleChange)
  {
    updateOverlayStyle();
  }
  QWidget::changeEvent(event);
}

void SequencePickerWidget::updateOverlayStyle()
{
  if (!overlay_)
  {
    return;
  }
  bool dark = isDarkTheme();
  overlay_->setStyleSheet(
      dark ? "QWidget#PickerOverlay { background-color: #333333; border: 1px solid #666666; }" :
             "QWidget#PickerOverlay { background-color: #ffffff; border: 1px solid #cccccc; }");
}

void SequencePickerWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  if (overlay_ && overlay_->isVisible())
  {
    repositionOverlay();
  }
}

bool SequencePickerWidget::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == window() && (event->type() == QEvent::Resize || event->type() == QEvent::Move))
  {
    if (overlay_ && overlay_->isVisible())
    {
      repositionOverlay();
    }
  }
  return QWidget::eventFilter(watched, event);
}

void SequencePickerWidget::repositionOverlay()
{
  if (!overlay_ || !calendar_button_)
  {
    return;
  }

  // Anchor: right edge of calendar button, at its bottom.
  QPoint anchor = calendar_button_->mapTo(
      window(), QPoint(calendar_button_->width(), calendar_button_->height()));

  // Overlay goes to the right of the anchor, below the button.
  int x = anchor.x() + 4;
  int y = anchor.y() + 2;

  // Clamp to window right edge.
  int overlay_width = overlay_->sizeHint().width();
  int max_x = window()->width() - overlay_width - 4;
  if (x > max_x)
  {
    x = max_x;
  }
  if (x < 0)
  {
    x = 0;
  }

  overlay_->move(x, y);
  overlay_->resize(overlay_->sizeHint());
  overlay_->raise();
}

// --- Preset logic ---

void SequencePickerWidget::onPresetClicked(int id)
{
  all_button_->setText("All");
  applyPreset(id);
  syncCalendarToFields();
  emitFilter();
}

void SequencePickerWidget::syncCalendarToFields()
{
  if (!dual_calendar_)
  {
    return;
  }
  const QString fmt = QStringLiteral("dd/MM/yyyy");
  QDate from = QDate::fromString(from_edit_->text(), fmt);
  QDate to = QDate::fromString(to_edit_->text(), fmt);
  dual_calendar_->setExternalRange(from, to);
}

void SequencePickerWidget::applyPreset(int id)
{
  QDate today = QDate::currentDate();
  QDate from, to;

  switch (id)
  {
    case kPresetAll:
      break;
    case kPresetPast24h:
      from = today.addDays(-1);
      to = today;
      break;
    case kPresetLast7Days:
      from = today.addDays(-6);
      to = today;
      break;
    case kPresetLastMonth: {
      QDate first_of_month(today.year(), today.month(), 1);
      to = first_of_month.addDays(-1);
      from = QDate(to.year(), to.month(), 1);
      break;
    }
  }

  updateFieldsFromPreset(from, to);
}

void SequencePickerWidget::updateFieldsFromPreset(const QDate& from, const QDate& to)
{
  const QString fmt = QStringLiteral("dd/MM/yyyy");
  QSignalBlocker fb(from_edit_);
  QSignalBlocker tb(to_edit_);
  from_edit_->setText(from.isValid() ? from.toString(fmt) : QString());
  to_edit_->setText(to.isValid() ? to.toString(fmt) : QString());
}

int SequencePickerWidget::matchingPreset() const
{
  const QString fmt = QStringLiteral("dd/MM/yyyy");
  QDate from = QDate::fromString(from_edit_->text(), fmt);
  QDate to = QDate::fromString(to_edit_->text(), fmt);
  QDate today = QDate::currentDate();

  if (from_edit_->text().isEmpty() && to_edit_->text().isEmpty())
  {
    return kPresetAll;
  }
  if (!from.isValid() || !to.isValid())
  {
    return -1;
  }
  if (from == today.addDays(-1) && to == today)
  {
    return kPresetPast24h;
  }
  if (from == today.addDays(-6) && to == today)
  {
    return kPresetLast7Days;
  }

  QDate first_of_month(today.year(), today.month(), 1);
  QDate last_of_prev = first_of_month.addDays(-1);
  QDate first_of_prev(last_of_prev.year(), last_of_prev.month(), 1);
  if (from == first_of_prev && to == last_of_prev)
  {
    return kPresetLastMonth;
  }

  return -1;
}

void SequencePickerWidget::checkCustomState()
{
  int preset = matchingPreset();
  if (preset >= 0)
  {
    all_button_->setText("All");
    preset_group_->button(preset)->setChecked(true);
  }
  else
  {
    all_button_->setText("Custom");
    all_button_->setChecked(true);
  }
  syncCalendarToFields();
  emitFilter();
}

// --- Toggle logic ---

void SequencePickerWidget::toggleCalendar()
{
  if (!overlay_)
  {
    return;
  }

  calendar_visible_ = !calendar_visible_;
  overlay_->setVisible(calendar_visible_);
  const auto pair = makeChevronPair(themeName());
  calendar_button_->setIcon(calendar_visible_ ? pair.expanded : pair.collapsed);

  if (calendar_visible_)
  {
    repositionOverlay();
  }
}

void SequencePickerWidget::onCalendarRangeCommitted(const QDate& from, const QDate& to)
{
  updateFieldsFromPreset(from, to);
  checkCustomState();
  if (time_picker_)
  {
    time_picker_->setFromDate(from);
    time_picker_->setToDate(to);
  }
}

void SequencePickerWidget::onCalendarRangePreview(const QDate& from, const QDate& to)
{
  // Update date fields to reflect the tentative range (for real-time filtering)
  // but don't update time picker dates — that only happens on commit.
  updateFieldsFromPreset(from, to);
  emitFilter();
}

void SequencePickerWidget::onTimeChanged()
{
  emitFilter();
}

// --- Build filter from current widget state ---

RangeFilter SequencePickerWidget::buildFilter() const
{
  RangeFilter f;

  const QString fmt = QStringLiteral("dd/MM/yyyy");
  QDate from = QDate::fromString(from_edit_->text(), fmt);
  QDate to = QDate::fromString(to_edit_->text(), fmt);
  if (from.isValid())
  {
    f.date_from = from;
  }
  if (to.isValid())
  {
    f.date_to = to;
  }

  if (time_picker_)
  {
    f.from_time = time_picker_->fromTime();
    // TimePickerWidget is minute-precision. Treat to_time as inclusive of the
    // whole selected minute by padding to hh:mm:59.999 — so "up to 11:59 PM"
    // actually includes all seconds of 23:59.
    QTime to = time_picker_->toTime();
    f.to_time = QTime(to.hour(), to.minute(), 59, 999);
    f.every_day = time_picker_->isEveryDay();
  }
  return f;
}

void SequencePickerWidget::emitFilter()
{
  emit filterChanged(buildFilter());
}
