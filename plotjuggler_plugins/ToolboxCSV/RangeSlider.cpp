
#include "RangeSlider.h"
#include <QDebug>

namespace
{

const int scHandleSideLength = 11;
const int scSliderBarHeight = 5;
const int scLeftRightMargin = 1;

}  // namespace

RangeSlider::RangeSlider(QWidget* aParent)
  : QWidget(aParent)
  , mMinimum(0)
  , mMaximum(100)
  , mLowerValue(0)
  , mUpperValue(100)
  , mMinReal(0.0)
  , mMaxReal(1.0)
  , mDecimals(3)
  , mScale(1000)
  , mMinTickPx(45)
  , mShowTicks(true)
  , mShowTickLabels(true)
  , mTooltipVisible(true)
  , mFirstHandlePressed(false)
  , mSecondHandlePressed(false)
  , mInterval(mMaximum - mMinimum)
  , mBackgroudColorEnabled(QColor(0x1E, 0x90, 0xFF))
  , mBackgroudColorDisabled(Qt::darkGray)
  , mBackgroudColor(mBackgroudColorEnabled)
  , orientation(Qt::Horizontal)
  , type(DoubleHandles)
{
  setMouseTracking(true);
}

RangeSlider::RangeSlider(Qt::Orientation ori, Options t, QWidget* aParent)
  : QWidget(aParent)
  , mMinimum(0)
  , mMaximum(100)
  , mLowerValue(0)
  , mUpperValue(100)
  , mFirstHandlePressed(false)
  , mSecondHandlePressed(false)
  , mInterval(mMaximum - mMinimum)
  , mBackgroudColorEnabled(QColor(0x1E, 0x90, 0xFF))
  , mBackgroudColorDisabled(Qt::darkGray)
  , mBackgroudColor(mBackgroudColorEnabled)
  , orientation(ori)
  , type(t)
{
  setMouseTracking(true);
}

void RangeSlider::paintEvent(QPaintEvent* aEvent)
{
  Q_UNUSED(aEvent);
  QPainter painter(this);

  // Background
  QRectF backgroundRect;
  if (orientation == Qt::Horizontal)
  {
    backgroundRect = QRectF(scLeftRightMargin, (height() - scSliderBarHeight) / 2,
                            width() - scLeftRightMargin * 2, scSliderBarHeight);
  }
  else
  {
    backgroundRect = QRectF((width() - scSliderBarHeight) / 2, scLeftRightMargin, scSliderBarHeight,
                            height() - scLeftRightMargin * 2);
  }

  QPen pen(Qt::gray, 0.8);
  painter.setPen(pen);
  painter.setRenderHint(QPainter::Qt4CompatiblePainting);
  QBrush backgroundBrush(QColor(0xD0, 0xD0, 0xD0));
  painter.setBrush(backgroundBrush);
  painter.drawRoundedRect(backgroundRect, 1, 1);

  if (mShowTicks)
  {
    drawTicks(painter, backgroundRect);
  }

  // First value handle rect
  pen.setColor(Qt::darkGray);
  pen.setWidth(0.5);
  painter.setPen(pen);
  painter.setRenderHint(QPainter::Antialiasing);
  QBrush handleBrush(QColor(0xFA, 0xFA, 0xFA));
  painter.setBrush(handleBrush);
  QRectF leftHandleRect = firstHandleRect();
  if (type.testFlag(LeftHandle))
  {
    painter.drawRoundedRect(leftHandleRect, 2, 2);
  }

  // Second value handle rect
  QRectF rightHandleRect = secondHandleRect();
  if (type.testFlag(RightHandle))
  {
    painter.drawRoundedRect(rightHandleRect, 2, 2);
  }

  // Handles
  painter.setRenderHint(QPainter::Antialiasing, false);
  QRectF selectedRect(backgroundRect);
  if (orientation == Qt::Horizontal)
  {
    selectedRect.setLeft(
        (type.testFlag(LeftHandle) ? leftHandleRect.right() : leftHandleRect.left()) + 0.5);
    selectedRect.setRight(
        (type.testFlag(RightHandle) ? rightHandleRect.left() : rightHandleRect.right()) - 0.5);
  }
  else
  {
    selectedRect.setTop(
        (type.testFlag(LeftHandle) ? leftHandleRect.bottom() : leftHandleRect.top()) + 0.5);
    selectedRect.setBottom(
        (type.testFlag(RightHandle) ? rightHandleRect.top() : rightHandleRect.bottom()) - 0.5);
  }
  QBrush selectedBrush(mBackgroudColor);
  painter.setBrush(selectedBrush);
  painter.drawRect(selectedRect);
}

QRectF RangeSlider::firstHandleRect() const
{
  float percentage = (mLowerValue - mMinimum) * 1.0 / mInterval;
  return handleRect(percentage * validLength() + scLeftRightMargin);
}

QRectF RangeSlider::secondHandleRect() const
{
  float percentage = (mUpperValue - mMinimum) * 1.0 / mInterval;
  return handleRect(percentage * validLength() + scLeftRightMargin +
                    (type.testFlag(LeftHandle) ? scHandleSideLength : 0));
}

QRectF RangeSlider::handleRect(int aValue) const
{
  if (orientation == Qt::Horizontal)
  {
    return QRect(aValue, (height() - scHandleSideLength) / 2, scHandleSideLength,
                 scHandleSideLength);
  }
  else
  {
    return QRect((width() - scHandleSideLength) / 2, aValue, scHandleSideLength,
                 scHandleSideLength);
  }
}

void RangeSlider::mousePressEvent(QMouseEvent* aEvent)
{
  if (aEvent->buttons() & Qt::LeftButton)
  {
    int posCheck, posMax, posValue, firstHandleRectPosValue, secondHandleRectPosValue;
    posCheck = (orientation == Qt::Horizontal) ? aEvent->pos().y() : aEvent->pos().x();
    posMax = (orientation == Qt::Horizontal) ? height() : width();
    posValue = (orientation == Qt::Horizontal) ? aEvent->pos().x() : aEvent->pos().y();
    firstHandleRectPosValue =
        (orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
    secondHandleRectPosValue =
        (orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();

    mSecondHandlePressed = secondHandleRect().contains(aEvent->pos());
    mFirstHandlePressed = !mSecondHandlePressed && firstHandleRect().contains(aEvent->pos());
    if (mFirstHandlePressed)
    {
      mDelta = posValue - (firstHandleRectPosValue + scHandleSideLength / 2);
    }
    else if (mSecondHandlePressed)
    {
      mDelta = posValue - (secondHandleRectPosValue + scHandleSideLength / 2);
    }

    if (posCheck >= 2 && posCheck <= posMax - 2)
    {
      int step = mInterval / 10 < 1 ? 1 : mInterval / 10;
      if (posValue < firstHandleRectPosValue)
      {
        setLowerValue(mLowerValue - step);
      }
      else if (((posValue > firstHandleRectPosValue + scHandleSideLength) ||
                !type.testFlag(LeftHandle)) &&
               ((posValue < secondHandleRectPosValue) || !type.testFlag(RightHandle)))
      {
        if (type.testFlag(DoubleHandles))
        {
          if (posValue - (firstHandleRectPosValue + scHandleSideLength) <
              (secondHandleRectPosValue - (firstHandleRectPosValue + scHandleSideLength)) / 2)
          {
            setLowerValue((mLowerValue + step < mUpperValue) ? mLowerValue + step : mUpperValue);
          }
          else
          {
            setUpperValue((mUpperValue - step > mLowerValue) ? mUpperValue - step : mLowerValue);
          }
        }
        else if (type.testFlag(LeftHandle))
        {
          setLowerValue((mLowerValue + step < mUpperValue) ? mLowerValue + step : mUpperValue);
        }
        else if (type.testFlag(RightHandle))
        {
          setUpperValue((mUpperValue - step > mLowerValue) ? mUpperValue - step : mLowerValue);
        }
      }
      else if (posValue > secondHandleRectPosValue + scHandleSideLength)
      {
        setUpperValue(mUpperValue + step);
      }
    }
  }

  maybeShowHandleTooltip(aEvent->globalPos(), aEvent->pos());
}

void RangeSlider::mouseMoveEvent(QMouseEvent* aEvent)
{
  if (aEvent->buttons() & Qt::LeftButton)
  {
    int posValue, firstHandleRectPosValue, secondHandleRectPosValue;
    posValue = (orientation == Qt::Horizontal) ? aEvent->pos().x() : aEvent->pos().y();
    firstHandleRectPosValue =
        (orientation == Qt::Horizontal) ? firstHandleRect().x() : firstHandleRect().y();
    secondHandleRectPosValue =
        (orientation == Qt::Horizontal) ? secondHandleRect().x() : secondHandleRect().y();

    if (mFirstHandlePressed && type.testFlag(LeftHandle))
    {
      if (posValue - mDelta + scHandleSideLength / 2 <= secondHandleRectPosValue)
      {
        setLowerValue((posValue - mDelta - scLeftRightMargin - scHandleSideLength / 2) * 1.0 /
                          validLength() * mInterval +
                      mMinimum);
      }
      else
      {
        setLowerValue(mUpperValue);
      }
    }
    else if (mSecondHandlePressed && type.testFlag(RightHandle))
    {
      if (firstHandleRectPosValue +
              scHandleSideLength * (type.testFlag(DoubleHandles) ? 1.5 : 0.5) <=
          posValue - mDelta)
      {
        setUpperValue((posValue - mDelta - scLeftRightMargin - scHandleSideLength / 2 -
                       (type.testFlag(DoubleHandles) ? scHandleSideLength : 0)) *
                          1.0 / validLength() * mInterval +
                      mMinimum);
      }
      else
      {
        setUpperValue(mLowerValue);
      }
    }
  }

  maybeShowHandleTooltip(aEvent->globalPos(), aEvent->pos());
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
  Q_UNUSED(aEvent);

  mFirstHandlePressed = false;
  mSecondHandlePressed = false;

  if (mShowHandleValueTooltip)
  {
    QToolTip::hideText();
    mTooltipVisible = false;
  }
}

void RangeSlider::changeEvent(QEvent* aEvent)
{
  if (aEvent->type() == QEvent::EnabledChange)
  {
    if (isEnabled())
    {
      mBackgroudColor = mBackgroudColorEnabled;
    }
    else
    {
      mBackgroudColor = mBackgroudColorDisabled;
    }
    update();
  }
}

void RangeSlider::leaveEvent(QEvent* e)
{
  QWidget::leaveEvent(e);
  QToolTip::hideText();
  mTooltipVisible = false;
}

QSize RangeSlider::minimumSizeHint() const
{
  return QSize(scHandleSideLength * 2 + scLeftRightMargin * 2, scHandleSideLength);
}

void RangeSlider::setLowerValue(int aLowerValue)
{
  if (aLowerValue > mMaximum)
  {
    aLowerValue = mMaximum;
  }

  if (aLowerValue < mMinimum)
  {
    aLowerValue = mMinimum;
  }

  mLowerValue = aLowerValue;
  emit lowerValueChanged(mLowerValue);

  update();
}

void RangeSlider::setUpperValue(int aUpperValue)
{
  if (aUpperValue > mMaximum)
  {
    aUpperValue = mMaximum;
  }

  if (aUpperValue < mMinimum)
  {
    aUpperValue = mMinimum;
  }

  mUpperValue = aUpperValue;
  emit upperValueChanged(mUpperValue);

  update();
}

void RangeSlider::setMinimum(int aMinimum)
{
  if (aMinimum <= mMaximum)
  {
    mMinimum = aMinimum;
  }
  else
  {
    int oldMax = mMaximum;
    mMinimum = oldMax;
    mMaximum = aMinimum;
  }
  mInterval = mMaximum - mMinimum;
  update();

  setLowerValue(mMinimum);
  setUpperValue(mMaximum);

  emit rangeChanged(mMinimum, mMaximum);
}

void RangeSlider::setMaximum(int aMaximum)
{
  if (aMaximum >= mMinimum)
  {
    mMaximum = aMaximum;
  }
  else
  {
    int oldMin = mMinimum;
    mMaximum = oldMin;
    mMinimum = aMaximum;
  }
  mInterval = mMaximum - mMinimum;
  update();

  setLowerValue(mMinimum);
  setUpperValue(mMaximum);

  emit rangeChanged(mMinimum, mMaximum);
}

int RangeSlider::validLength() const
{
  int len = (orientation == Qt::Horizontal) ? width() : height();
  return len - scLeftRightMargin * 2 - scHandleSideLength * (type.testFlag(DoubleHandles) ? 2 : 1);
}

void RangeSlider::setOptions(Options t)
{
  type = t;
  update();
}

void RangeSlider::setMinTickPixelSpacing(int px)
{
  mMinTickPx = px;
  update();
}

void RangeSlider::setShowTickLabels(bool on)
{
  mShowTickLabels = on;
  update();
}

void RangeSlider::setShowTicks(bool on)
{
  mShowTicks = on;
  update();
}

bool RangeSlider::showTicks() const
{
  return mShowTicks;
}

int RangeSlider::niceStep(int raw) const
{
  if (raw <= 1)
  {
    return 1;
  }

  int p = 1;
  while (p * 10 <= raw)
  {
    p *= 10;
  }

  int d = raw / p;
  int step = 1;
  if (d <= 1)
  {
    step = 1;
  }
  else if (d <= 2)
  {
    step = 2;
  }
  else if (d <= 5)
  {
    step = 5;
  }
  else
  {
    step = 10;
  }

  return step * p;
}

int RangeSlider::firstTick(int min, int step) const
{
  if (step <= 0)
  {
    return min;
  }
  int r = min % step;
  return (r == 0) ? min : (min + (step - r));
}

void RangeSlider::drawTicks(QPainter& painter, const QRectF& backgroundRect)
{
  if (mInterval <= 0)
  {
    return;
  }

  int pxLen = validLength();
  if (pxLen <= 0)
  {
    return;
  }

  int approxCount = std::max(2, pxLen / std::max(10, mMinTickPx));
  int idealStep = std::max(1, (mMaximum - mMinimum) / approxCount);
  int step = niceStep(idealStep);

  int start = firstTick(mMinimum, step);

  QFontMetrics fm(painter.font());
  int majorLen = 10;
  int minorLen = 6;

  int minorStep = step / 2;
  if (minorStep < 1)
  {
    minorStep = 1;
  }

  auto valueToPos = [&](int value) {
    const float percentage = (value - mMinimum) * 1.0f / mInterval;
    const int offset = scLeftRightMargin + (type.testFlag(DoubleHandles) ? scHandleSideLength : 0);
    const int base = static_cast<int>(percentage * pxLen) + offset;
    return base;
  };

  painter.setPen(Qt::gray);

  for (int v = start; v <= mMaximum; v += minorStep)
  {
    bool major = ((v - start) % step) == 0;

    int pos = valueToPos(v);

    if (orientation == Qt::Horizontal)
    {
      int y = backgroundRect.bottom();
      painter.drawLine(pos, y, pos, y + (major ? majorLen : minorLen));

      if (major && mShowTickLabels)
      {
        QString txt = QString::number(v);
        int w = fm.horizontalAdvance(txt);
        painter.drawText(pos - w / 2, y + majorLen + fm.ascent() + 2, txt);
      }
    }
    else
    {
      int x = backgroundRect.right();
      painter.drawLine(x, pos, x + (major ? majorLen : minorLen), pos);

      if (major && mShowTickLabels)
      {
        QString txt = QString::number(v);
        painter.drawText(x + majorLen + 4, pos + fm.ascent() / 2, txt);
      }
    }
  }
}

void RangeSlider::setShowHandleValueTooltip(bool on)
{
  mShowHandleValueTooltip = on;
  if (!on)
  {
    QToolTip::hideText();
    mTooltipVisible = false;
  }
}

bool RangeSlider::showHandleValueTooltip() const
{
  return mShowHandleValueTooltip;
}

QString RangeSlider::handleValueText(bool left) const
{
  if (mDecimals <= 0)
  {
    return QString::number(left ? mLowerValue : mUpperValue);
  }

  double v = left ? lowerValueReal() : upperValueReal();
  return QString::number(v, 'f', mDecimals);
}

void RangeSlider::maybeShowHandleTooltip(const QPoint& globalPos, const QPoint& localPos)
{
  if (!mShowHandleValueTooltip)
  {
    return;
  }

  bool overLeft = type.testFlag(LeftHandle) && firstHandleRect().contains(localPos);
  bool overRight = type.testFlag(RightHandle) && secondHandleRect().contains(localPos);

  if (mFirstHandlePressed && type.testFlag(LeftHandle))
  {
    overLeft = true;
  }
  if (mSecondHandlePressed && type.testFlag(RightHandle))
  {
    overRight = true;
  }

  if (overLeft)
  {
    QToolTip::showText(globalPos, handleValueText(true), this);
    mTooltipVisible = true;
  }
  else if (overRight)
  {
    QToolTip::showText(globalPos, handleValueText(false), this);
    mTooltipVisible = true;
  }
  else if (mTooltipVisible)
  {
    QToolTip::hideText();
    mTooltipVisible = false;
  }
}

int RangeSlider::toInt(double v) const
{
  if (v < mMinReal)
  {
    v = mMinReal;
  }
  if (v > mMaxReal)
  {
    v = mMaxReal;
  }
  return int((v - mMinReal) * mScale + 0.5);
}

double RangeSlider::toReal(int v) const
{
  return mMinReal + (v * 1.0 / mScale);
}

void RangeSlider::setRangeReal(double minV, double maxV, int decimals)
{
  if (minV > maxV)
  {
    std::swap(minV, maxV);
  }

  mMinReal = minV;
  mMaxReal = maxV;

  double span = mMaxReal - mMinReal;
  if (span <= 0.0)
  {
    span = 1.0;
  }

  int d = std::max(0, decimals);
  long long s = 1;
  for (int i = 0; i < d; i++)
  {
    s *= 10;
  }
  while (d > 0 && span * double(s) > double(std::numeric_limits<int>::max()))
  {
    s /= 10;
    d--;
  }

  mDecimals = d;
  mScale = 1;
  for (int i = 0; i < mDecimals; i++)
  {
    mScale *= 10;
  }

  int imin = 0;
  int imax = std::max(1, int(span * double(mScale) + 0.5));

  setMinimum(imin);
  setMaximum(imax);
}

void RangeSlider::setLowerValueReal(double v)
{
  setLowerValue(toInt(v));
}

void RangeSlider::setUpperValueReal(double v)
{
  setUpperValue(toInt(v));
}

double RangeSlider::lowerValueReal() const
{
  return toReal(mLowerValue);
}

double RangeSlider::upperValueReal() const
{
  return toReal(mUpperValue);
}

int RangeSlider::decimals() const
{
  return mDecimals;
}
