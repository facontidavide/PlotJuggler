/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "toast_notification.h"

#include <QHBoxLayout>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPainterPath>

ToastNotification::ToastNotification(const QString& message, const QPixmap& icon, int timeout_ms,
                                     QWidget* parent)
  : QFrame(parent)
  , _icon_label(nullptr)
  , _message_label(nullptr)
  , _close_button(nullptr)
  , _slide_animation(nullptr)
  , _timeout_timer(nullptr)
  , _timeout_ms(timeout_ms)
  , _is_closing(false)
{
  setObjectName("ToastNotification");
  setupUI();
  setupAnimation();

  setMessage(message);
  if (!icon.isNull())
  {
    setIcon(icon);
  }
}

ToastNotification::~ToastNotification()
{
  if (_slide_animation)
  {
    _slide_animation->stop();
  }
  if (_timeout_timer)
  {
    _timeout_timer->stop();
  }
}

void ToastNotification::setupUI()
{
  // Frame styling
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Raised);
  setMinimumHeight(40);  // Minimum for text-only toasts
  setMaximumWidth(400);

  // Main horizontal layout
  _layout = new QHBoxLayout(this);
  _layout->setContentsMargins(8, 8, 8, 8);
  _layout->setSpacing(12);

  // Icon label (optional, hidden by default)
  _icon_label = new QLabel(this);
  _icon_label->setObjectName("toastIcon");
  _icon_label->setFixedSize(ICON_SIZE, ICON_SIZE);
  _icon_label->setScaledContents(false);  // We handle scaling ourselves for rounded corners
  _icon_label->setVisible(false);
  _layout->addWidget(_icon_label);

  // Message label (supports HTML/rich text with clickable links)
  _message_label = new QLabel(this);
  _message_label->setObjectName("toastMessage");
  _message_label->setWordWrap(true);
  _message_label->setTextFormat(Qt::RichText);
  _message_label->setOpenExternalLinks(true);
  _message_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  _layout->addWidget(_message_label, 1);

  // Close button
  _close_button = new QPushButton(this);
  _close_button->setObjectName("toastCloseButton");
  _close_button->setText("\u00D7");  // Unicode multiplication sign (×)
  _close_button->setFixedSize(24, 24);
  _close_button->setFlat(true);
  _close_button->setCursor(Qt::PointingHandCursor);
  _close_button->setFocusPolicy(Qt::NoFocus);
  _layout->addWidget(_close_button, 0, Qt::AlignTop);

  connect(_close_button, &QPushButton::clicked, this, &ToastNotification::onCloseClicked);

  // Timeout timer
  if (_timeout_ms > 0)
  {
    _timeout_timer = new QTimer(this);
    _timeout_timer->setSingleShot(true);
    connect(_timeout_timer, &QTimer::timeout, this, &ToastNotification::onTimeout);
  }
}

void ToastNotification::setupAnimation()
{
  _slide_animation = new QPropertyAnimation(this, "pos", this);
  _slide_animation->setDuration(ANIMATION_DURATION_MS);
  _slide_animation->setEasingCurve(QEasingCurve::OutCubic);

  connect(_slide_animation, &QPropertyAnimation::finished, this,
          &ToastNotification::onAnimationFinished);
}

void ToastNotification::showAnimated()
{
  // Get target position (set by ToastManager)
  QPoint endPos = pos();

  // Start from off-screen right
  QPoint startPos = endPos;
  startPos.setX(endPos.x() + width() + 50);

  // Move to start position, then animate to end
  move(startPos);
  show();

  _slide_animation->setStartValue(startPos);
  _slide_animation->setEndValue(endPos);
  _slide_animation->start();

  // Start timeout timer after animation completes
  if (_timeout_timer && _timeout_ms > 0)
  {
    QTimer::singleShot(ANIMATION_DURATION_MS, this, [this]() {
      if (_timeout_timer && !_is_closing)
      {
        _timeout_timer->start(_timeout_ms);
      }
    });
  }
}

void ToastNotification::hideAnimated()
{
  if (_is_closing)
  {
    return;
  }
  _is_closing = true;

  // Stop timeout timer
  if (_timeout_timer)
  {
    _timeout_timer->stop();
  }

  // Animate sliding out to the right
  QPoint startPos = pos();
  QPoint endPos = startPos;
  endPos.setX(startPos.x() + width() + 50);

  _slide_animation->setStartValue(startPos);
  _slide_animation->setEndValue(endPos);
  _slide_animation->start();
}

QString ToastNotification::message() const
{
  return _message_label->text();
}

void ToastNotification::setMessage(const QString& message)
{
  _message_label->setText(message);
}

void ToastNotification::setIcon(const QPixmap& icon)
{
  if (icon.isNull())
  {
    _icon_label->setVisible(false);
    // Restore normal margins
    _layout->setContentsMargins(8, 8, 8, 8);
  }
  else
  {
    // Scale and round the icon
    QPixmap scaled =
        icon.scaled(ICON_SIZE, ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap rounded = createRoundedPixmap(scaled, BORDER_RADIUS);
    _icon_label->setPixmap(rounded);
    _icon_label->setVisible(true);
    // Remove left margin so icon touches the edge, round the icon's left corners
    _layout->setContentsMargins(0, 0, 8, 0);
  }
}

QPixmap ToastNotification::createRoundedPixmap(const QPixmap& source, int radius)
{
  if (source.isNull())
    return source;

  QPixmap result(source.size());
  result.fill(Qt::transparent);

  QPainter painter(&result);
  painter.setRenderHint(QPainter::Antialiasing);

  // Create a path with rounded corners only on the left side
  QPainterPath path;
  QRectF rect(0, 0, source.width(), source.height());

  // Top-left rounded corner
  path.moveTo(rect.left() + radius, rect.top());
  // Top edge to top-right (no rounding on right side)
  path.lineTo(rect.right(), rect.top());
  // Right edge
  path.lineTo(rect.right(), rect.bottom());
  // Bottom edge to bottom-left
  path.lineTo(rect.left() + radius, rect.bottom());
  // Bottom-left rounded corner
  path.arcTo(rect.left(), rect.bottom() - 2 * radius, 2 * radius, 2 * radius, 270, -90);
  // Left edge
  path.lineTo(rect.left(), rect.top() + radius);
  // Top-left rounded corner
  path.arcTo(rect.left(), rect.top(), 2 * radius, 2 * radius, 180, -90);
  path.closeSubpath();

  painter.setClipPath(path);
  painter.drawPixmap(0, 0, source);

  return result;
}

void ToastNotification::updateTargetPosition(const QPoint& target)
{
  if (_slide_animation->state() == QAbstractAnimation::Running && !_is_closing)
  {
    // Animation is in progress - update the end value
    _slide_animation->stop();
    QPoint currentPos = pos();
    _slide_animation->setStartValue(currentPos);
    _slide_animation->setEndValue(target);
    _slide_animation->start();
  }
  else if (!_is_closing)
  {
    // No animation running, just move
    move(target);
  }
}

bool ToastNotification::isAnimatingIn() const
{
  return _slide_animation->state() == QAbstractAnimation::Running && !_is_closing;
}

void ToastNotification::showEvent(QShowEvent* event)
{
  QFrame::showEvent(event);
}

void ToastNotification::onCloseClicked()
{
  hideAnimated();
}

void ToastNotification::onAnimationFinished()
{
  if (_is_closing)
  {
    emit closed();
    deleteLater();
  }
}

void ToastNotification::onTimeout()
{
  hideAnimated();
}
