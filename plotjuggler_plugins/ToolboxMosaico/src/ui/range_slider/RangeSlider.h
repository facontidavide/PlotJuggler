#pragma once

#include <QToolTip>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

#include <functional>

class RangeSlider : public QWidget
{
  Q_OBJECT
  Q_ENUMS(RangeSliderTypes)

public:
  enum Option
  {
    NoHandle = 0x0,
    LeftHandle = 0x1,
    RightHandle = 0x2,
    DoubleHandles = LeftHandle | RightHandle
  };
  Q_DECLARE_FLAGS(Options, Option)

  RangeSlider(QWidget* aParent = Q_NULLPTR);
  RangeSlider(Qt::Orientation ori, Options t = DoubleHandles, QWidget* aParent = Q_NULLPTR);

  QSize minimumSizeHint() const override;

  int GetMinimun() const;
  void SetMinimum(int aMinimum);

  int GetMaximun() const;
  void SetMaximum(int aMaximum);

  int GetLowerValue() const;
  void SetLowerValue(int aLowerValue);

  int GetUpperValue() const;
  void SetUpperValue(int aUpperValue);

  void SetRange(int aMinimum, int aMaximum);

  void setOptions(Options t);

  void setMinTickPixelSpacing(int px);

  void setShowTickLabels(bool on);
  void setShowTicks(bool on);

  void setShowHandleValueTooltip(bool on);
  bool showHandleValueTooltip() const;

  // Floating labels — painted above handles during drag.
  void setFloatingLabelsVisible(bool on);
  bool floatingLabelsVisible() const;

  // Custom formatters for floating labels (default: decimal number).
  // Handle formatter receives the real value for one handle.
  void setLabelFormatter(std::function<QString(double)> formatter);
  // Center formatter receives (lower, upper) real values. Return empty to hide.
  void setCenterLabelFormatter(std::function<QString(double, double)> formatter);

  bool showTicks() const;

  void setRangeReal(double minV, double maxV, int decimals);
  void setLowerValueReal(double v);
  void setUpperValueReal(double v);
  double lowerValueReal() const;
  double upperValueReal() const;
  int decimals() const;
  int toInt(double v) const;
  double toReal(int v) const;

protected:
  void paintEvent(QPaintEvent* aEvent) override;
  void mousePressEvent(QMouseEvent* aEvent) override;
  void mouseMoveEvent(QMouseEvent* aEvent) override;
  void mouseReleaseEvent(QMouseEvent* aEvent) override;
  void changeEvent(QEvent* aEvent) override;
  void leaveEvent(QEvent* e) override;

  QRectF firstHandleRect() const;
  QRectF secondHandleRect() const;
  QRectF handleRect(int aValue) const;

signals:
  void lowerValueChanged(int aLowerValue);
  void upperValueChanged(int aUpperValue);
  void rangeChanged(int aMin, int aMax);

public slots:
  void setLowerValue(int aLowerValue);
  void setUpperValue(int aUpperValue);
  void setMinimum(int aMinimum);
  void setMaximum(int aMaximum);

private:
  Q_DISABLE_COPY(RangeSlider)
  int validLength() const;

  int mMinimum = 0;
  int mMaximum = 100;
  int mLowerValue = 0;
  int mUpperValue = 100;
  bool mFirstHandlePressed = false;
  bool mSecondHandlePressed = false;
  bool mRangeDragActive = false;
  int mRangeDragStartPos = 0;
  int mRangeDragLowerStart = 0;
  int mRangeDragUpperStart = 0;
  int mInterval = 100;
  int mDelta = 0;
  QColor mBackgroudColorEnabled{0x1E, 0x90, 0xFF};
  QColor mBackgroudColorDisabled{Qt::darkGray};
  QColor mBackgroudColor{mBackgroudColorEnabled};
  Qt::Orientation orientation = Qt::Horizontal;
  Options type = DoubleHandles;

  int mMinTickPx = 45;
  bool mShowTicks = true;
  bool mShowTickLabels = true;

  void drawTicks(QPainter& painter, const QRectF& backgroundRect);
  int niceStep(int raw) const;
  int firstTick(int min, int step) const;

  bool mShowHandleValueTooltip = true;
  bool mTooltipVisible = false;

  bool mFloatingLabels = false;
  std::function<QString(double)> mLabelFormatter;
  std::function<QString(double, double)> mCenterLabelFormatter;

  void drawFloatingLabels(QPainter& painter);
  QString formatHandleValue(double value) const;

  void maybeShowHandleTooltip(const QPoint& globalPos, const QPoint& localPos);
  QString handleValueText(bool left) const;

  double mMinReal = 0.0;
  double mMaxReal = 1.0;
  int mDecimals = 3;
  int mScale = 1000;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RangeSlider::Options)
