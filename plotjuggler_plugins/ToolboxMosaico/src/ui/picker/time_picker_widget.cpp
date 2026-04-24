#include "time_picker_widget.h"

#include <QHBoxLayout>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>

TimePickerWidget::TimePickerWidget(QWidget* parent) : QWidget(parent)
{
  auto* layout = new QHBoxLayout(this);

  auto makeTimeGroup = [&](QLabel*& dateLabel, QSpinBox*& hour, QSpinBox*& minute,
                           QComboBox*& ampm) {
    dateLabel = new QLabel("---");

    hour = new QSpinBox;
    hour->setRange(1, 12);
    hour->setWrapping(true);
    hour->setFixedWidth(45);
    hour->setAlignment(Qt::AlignCenter);

    auto* colon = new QLabel(":");

    minute = new QSpinBox;
    minute->setRange(0, 59);
    minute->setWrapping(true);
    minute->setFixedWidth(45);
    minute->setAlignment(Qt::AlignCenter);
    minute->setSpecialValueText("00");

    ampm = new QComboBox;
    ampm->addItems({ "AM", "PM" });
    ampm->setFixedWidth(50);

    layout->addWidget(dateLabel);
    layout->addWidget(hour);
    layout->addWidget(colon);
    layout->addWidget(minute);
    layout->addWidget(ampm);
  };

  // From time group
  makeTimeGroup(from_date_label_, from_hour_, from_minute_, from_am_pm_);

  // Default: 12:00 AM
  from_hour_->setValue(12);
  from_minute_->setValue(0);
  from_am_pm_->setCurrentIndex(0);  // AM

  // Spacer between from and to
  layout->addSpacing(16);

  // To time group
  makeTimeGroup(to_date_label_, to_hour_, to_minute_, to_am_pm_);

  // Default: 11:59 PM
  to_hour_->setValue(11);
  to_minute_->setValue(59);
  to_am_pm_->setCurrentIndex(1);  // PM

  // Every day checkbox
  layout->addSpacing(16);
  every_day_ = new QCheckBox("Every day");
  layout->addWidget(every_day_);

  layout->addStretch();

  // Connect all changes to timeChanged signal
  auto emitChanged = [this]() { emit timeChanged(); };
  connect(from_hour_, QOverload<int>::of(&QSpinBox::valueChanged), this, emitChanged);
  connect(from_minute_, QOverload<int>::of(&QSpinBox::valueChanged), this, emitChanged);
  connect(from_am_pm_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, emitChanged);
  connect(to_hour_, QOverload<int>::of(&QSpinBox::valueChanged), this, emitChanged);
  connect(to_minute_, QOverload<int>::of(&QSpinBox::valueChanged), this, emitChanged);
  connect(to_am_pm_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, emitChanged);
  connect(every_day_, &QCheckBox::toggled, this, emitChanged);
}

void TimePickerWidget::setFromDate(const QDate& date)
{
  from_date_label_->setText(date.isValid() ? date.toString("ddd dd-MM-yy") : QStringLiteral("---"));
}

void TimePickerWidget::setToDate(const QDate& date)
{
  to_date_label_->setText(date.isValid() ? date.toString("ddd dd-MM-yy") : QStringLiteral("---"));
}

QTime TimePickerWidget::timeFrom12Hour(int hour12, int minute, const QString& ampm) const
{
  int h = hour12 % 12;  // 12 -> 0
  if (ampm == "PM")
  {
    h += 12;
  }
  return QTime(h, minute);
}

QTime TimePickerWidget::fromTime() const
{
  return timeFrom12Hour(from_hour_->value(), from_minute_->value(), from_am_pm_->currentText());
}

QTime TimePickerWidget::toTime() const
{
  return timeFrom12Hour(to_hour_->value(), to_minute_->value(), to_am_pm_->currentText());
}

bool TimePickerWidget::isEveryDay() const
{
  return every_day_->isChecked();
}
