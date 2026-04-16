#pragma once

#include <QToolTip>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

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

  void setOptions(Options t);

  void setMinTickPixelSpacing(int px);

  void setShowTickLabels(bool on);
  void setShowTicks(bool on);

  void setShowHandleValueTooltip(bool on);
  bool showHandleValueTooltip() const;

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

  int mMinimum;
  int mMaximum;
  int mLowerValue;
  int mUpperValue;
  bool mFirstHandlePressed;
  bool mSecondHandlePressed;
  int mInterval;
  int mDelta;
  QColor mBackgroudColorEnabled;
  QColor mBackgroudColorDisabled;
  QColor mBackgroudColor;
  Qt::Orientation orientation;
  Options type;

  int mMinTickPx;
  bool mShowTicks;
  bool mShowTickLabels;

  void drawTicks(QPainter& painter, const QRectF& backgroundRect);
  int niceStep(int raw) const;
  int firstTick(int min, int step) const;

  bool mShowHandleValueTooltip = true;
  bool mTooltipVisible = false;

  void maybeShowHandleTooltip(const QPoint& globalPos, const QPoint& localPos);
  QString handleValueText(bool left) const;

  double mMinReal;
  double mMaxReal;
  int mDecimals;
  int mScale;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(RangeSlider::Options)
