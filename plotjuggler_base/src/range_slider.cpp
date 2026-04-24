
#include "PlotJuggler/range_slider.h"
#include <QDebug>

namespace
{

const int scHandleSideLength = 11;
const int scSliderBarHeight = 5;
const int scLeftRightMargin = 1;

}  // namespace

RangeSlider::RangeSlider(QWidget* aParent) : QWidget(aParent)
{
  setMouseTracking(true);
}

RangeSlider::RangeSlider(Qt::Orientation ori, Options t, QWidget* aParent)
  : QWidget(aParent), orientation(ori), type(t)
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

  const QPalette& pal = palette();
  QPen pen(pal.color(QPalette::Mid), 0.8);
  painter.setPen(pen);
  // Qt4CompatiblePainting removed in Qt6; Antialiasing is sufficient.
  painter.setBrush(pal.color(QPalette::Dark));
  painter.drawRoundedRect(backgroundRect, 1, 1);

  if (mShowTicks)
  {
    drawTicks(painter, backgroundRect);
  }

  // First value handle rect
  pen.setColor(pal.color(QPalette::Mid));
  pen.setWidth(0.5);
  painter.setPen(pen);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QColor(0x3e, 0x47, 0x50));
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

  if (mFloatingLabels)
  {
    drawFloatingLabels(painter);
  }
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

    // Floating labels double as hit-test targets. When visible, a click
    // on a handle's label behaves like a click on the handle itself, and
    // a click on the center label starts a range drag. Label rects are
    // published by drawFloatingLabels on each paint.
    const bool on_lower_label =
        mFloatingLabels && !mLowerLabelRect.isNull() && mLowerLabelRect.contains(aEvent->pos());
    const bool on_upper_label =
        mFloatingLabels && !mUpperLabelRect.isNull() && mUpperLabelRect.contains(aEvent->pos());
    const bool on_center_label =
        mFloatingLabels && !mCenterLabelRect.isNull() && mCenterLabelRect.contains(aEvent->pos());

    mSecondHandlePressed = on_upper_label || (!on_lower_label && !on_center_label &&
                                              secondHandleRect().contains(aEvent->pos()));
    mFirstHandlePressed = on_lower_label || (!mSecondHandlePressed && !on_center_label &&
                                             firstHandleRect().contains(aEvent->pos()));
    mRangeDragActive = false;

    if (mFirstHandlePressed)
    {
      mDelta = posValue - (firstHandleRectPosValue + scHandleSideLength / 2);
    }
    else if (mSecondHandlePressed)
    {
      mDelta = posValue - (secondHandleRectPosValue + scHandleSideLength / 2);
    }
    else if (on_center_label && type.testFlag(DoubleHandles))
    {
      // Grab the span by its center label — same drag behavior as clicking
      // the track between the two handles.
      mRangeDragActive = true;
      mRangeDragStartPos = posValue;
      mRangeDragLowerStart = mLowerValue;
      mRangeDragUpperStart = mUpperValue;
    }
    else if (type.testFlag(DoubleHandles) &&
             posValue > firstHandleRectPosValue + scHandleSideLength &&
             posValue < secondHandleRectPosValue && posCheck >= 2 && posCheck <= posMax - 2)
    {
      // Clicked between the two handles — start range drag.
      mRangeDragActive = true;
      mRangeDragStartPos = posValue;
      mRangeDragLowerStart = mLowerValue;
      mRangeDragUpperStart = mUpperValue;
    }
    else if (posCheck >= 2 && posCheck <= posMax - 2)
    {
      int step = mInterval / 10 < 1 ? 1 : mInterval / 10;
      if (posValue < firstHandleRectPosValue)
      {
        setLowerValue(mLowerValue - step);
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

    if (mRangeDragActive)
    {
      // Drag both handles together, preserving the span.
      int pixelDelta = posValue - mRangeDragStartPos;
      int valueDelta = static_cast<int>(pixelDelta * 1.0 / validLength() * mInterval);
      int newLower = mRangeDragLowerStart + valueDelta;
      int newUpper = mRangeDragUpperStart + valueDelta;

      // Clamp to [mMinimum, mMaximum].
      if (newLower < mMinimum)
      {
        newUpper += (mMinimum - newLower);
        newLower = mMinimum;
      }
      if (newUpper > mMaximum)
      {
        newLower -= (newUpper - mMaximum);
        newUpper = mMaximum;
      }
      newLower = std::max(newLower, mMinimum);
      newUpper = std::min(newUpper, mMaximum);

      setLowerValue(newLower);
      setUpperValue(newUpper);
    }
    else if (mFirstHandlePressed && type.testFlag(LeftHandle))
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

  update();
  maybeShowHandleTooltip(aEvent->globalPos(), aEvent->pos());
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* aEvent)
{
  Q_UNUSED(aEvent);

  mFirstHandlePressed = false;
  mSecondHandlePressed = false;
  mRangeDragActive = false;
  update();

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
  int h = scHandleSideLength;
  if (mFloatingLabels)
  {
    // One label row above (handle labels) + one below (center duration).
    QFontMetrics fm(font());
    int label_row = fm.height() + 6 + 4;  // label_height + gap
    h += label_row * 2;
  }
  return QSize(scHandleSideLength * 2 + scLeftRightMargin * 2, h);
}

int RangeSlider::GetMinimun() const
{
  return mMinimum;
}

void RangeSlider::SetMinimum(int aMinimum)
{
  setMinimum(aMinimum);
}

int RangeSlider::GetMaximun() const
{
  return mMaximum;
}

void RangeSlider::SetMaximum(int aMaximum)
{
  setMaximum(aMaximum);
}

int RangeSlider::GetLowerValue() const
{
  return mLowerValue;
}

void RangeSlider::SetLowerValue(int aLowerValue)
{
  setLowerValue(aLowerValue);
}

int RangeSlider::GetUpperValue() const
{
  return mUpperValue;
}

void RangeSlider::SetUpperValue(int aUpperValue)
{
  setUpperValue(aUpperValue);
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

void RangeSlider::SetRange(int aMinimum, int mMaximum)
{
  setMinimum(aMinimum);
  setMaximum(mMaximum);
  mDecimals = 0;
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
  while (d > 0)
  {
    long long s = 1;
    for (int i = 0; i < d; i++)
    {
      s *= 10;
    }
    if (span * double(s) <= double(std::numeric_limits<int>::max()))
    {
      break;
    }
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

void RangeSlider::setFloatingLabelsVisible(bool on)
{
  mFloatingLabels = on;
  update();
}

bool RangeSlider::floatingLabelsVisible() const
{
  return mFloatingLabels;
}

void RangeSlider::setLabelFormatter(std::function<QString(double)> formatter)
{
  mLabelFormatter = std::move(formatter);
  update();
}

void RangeSlider::setCenterLabelFormatter(std::function<QString(double, double)> formatter)
{
  mCenterLabelFormatter = std::move(formatter);
  update();
}

QString RangeSlider::formatHandleValue(double value) const
{
  if (mLabelFormatter)
  {
    return mLabelFormatter(value);
  }
  return handleValueText(value == lowerValueReal());
}

void RangeSlider::drawFloatingLabels(QPainter& painter)
{
  // Reset hit-test rects — repopulated below if the corresponding label
  // actually gets drawn. Empty rect means "no label there, ignore clicks."
  mLowerLabelRect = QRect();
  mUpperLabelRect = QRect();
  mCenterLabelRect = QRect();

  if (orientation != Qt::Horizontal)
  {
    return;  // Only horizontal for now.
  }

  painter.setRenderHint(QPainter::Antialiasing);
  QFont label_font = font();
  painter.setFont(label_font);
  QFontMetrics fm(label_font);

  const int label_height = fm.height() + 6;  // padding
  // Position labels from the top of the widget, above the handle area.
  const int handle_top = (height() - scHandleSideLength) / 2;
  const int label_y = handle_top - label_height - 2;

  auto drawLabel = [&](const QRectF& handle_rect, const QString& text) -> QRect {
    if (text.isEmpty())
    {
      return QRect();
    }
    int text_width = fm.horizontalAdvance(text) + 8;
    int label_x = static_cast<int>(handle_rect.center().x()) - text_width / 2;

    // Clamp to widget bounds.
    label_x = std::max(0, std::min(label_x, width() - text_width));

    QRect rect(label_x, label_y, text_width, label_height);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 50, 220));
    painter.drawRoundedRect(rect, 4, 4);

    painter.setPen(Qt::white);
    painter.drawText(rect, Qt::AlignCenter, text);
    return rect;
  };

  // Left handle label.
  if (type.testFlag(LeftHandle))
  {
    mLowerLabelRect =
        drawLabel(firstHandleRect(), formatHandleValue(static_cast<double>(mLowerValue)));
  }

  // Right handle label.
  if (type.testFlag(RightHandle))
  {
    mUpperLabelRect =
        drawLabel(secondHandleRect(), formatHandleValue(static_cast<double>(mUpperValue)));
  }

  // Center label (duration).
  if (mCenterLabelFormatter)
  {
    QString center_text =
        mCenterLabelFormatter(static_cast<double>(mLowerValue), static_cast<double>(mUpperValue));
    if (!center_text.isEmpty())
    {
      QRectF left_rect = firstHandleRect();
      QRectF right_rect = secondHandleRect();
      double center_x = (left_rect.center().x() + right_rect.center().x()) / 2.0;
      int text_width = fm.horizontalAdvance(center_text) + 8;
      int cx = static_cast<int>(center_x) - text_width / 2;
      cx = std::max(0, std::min(cx, width() - text_width));

      // Paint below the slider bar.
      const int handle_bottom = (height() + scHandleSideLength) / 2;
      int center_label_y = handle_bottom + 2;
      QRect rect(cx, center_label_y, text_width, label_height);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QColor(30, 80, 160, 220));
      painter.drawRoundedRect(rect, 4, 4);

      painter.setPen(Qt::white);
      painter.drawText(rect, Qt::AlignCenter, center_text);
      mCenterLabelRect = rect;
    }
  }
}
