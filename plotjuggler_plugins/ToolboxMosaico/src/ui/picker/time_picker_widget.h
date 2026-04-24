#pragma once

#include <QWidget>
#include <QDate>
#include <QTime>

class QSpinBox;
class QComboBox;
class QLabel;
class QCheckBox;

class TimePickerWidget : public QWidget
{
  Q_OBJECT

public:
  explicit TimePickerWidget(QWidget* parent = nullptr);

  void setFromDate(const QDate& date);
  void setToDate(const QDate& date);

  QTime fromTime() const;
  QTime toTime() const;
  bool isEveryDay() const;

signals:
  void timeChanged();

private:
  QTime timeFrom12Hour(int hour12, int minute, const QString& ampm) const;

  QLabel* from_date_label_;
  QSpinBox* from_hour_;
  QSpinBox* from_minute_;
  QComboBox* from_am_pm_;

  QLabel* to_date_label_;
  QSpinBox* to_hour_;
  QSpinBox* to_minute_;
  QComboBox* to_am_pm_;

  QCheckBox* every_day_;
};
