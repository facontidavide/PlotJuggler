/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TOAST_NOTIFICATION_H
#define TOAST_NOTIFICATION_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPixmap>
#include <QString>

class QHBoxLayout;

class ToastNotification : public QFrame
{
  Q_OBJECT

public:
  /**
   * @brief Construct a toast notification widget
   * @param message The text message to display
   * @param icon Optional icon to display on the left (56x56 pixels)
   * @param timeout_ms Auto-dismiss timeout in milliseconds (0 = no auto-dismiss)
   * @param parent Parent widget
   */
  explicit ToastNotification(const QString& message, const QPixmap& icon = QPixmap(),
                             int timeout_ms = 10000, QWidget* parent = nullptr);

  ~ToastNotification() override;

  /// Start the slide-in animation from the right
  void showAnimated();

  /// Start the slide-out animation to the right, then close
  void hideAnimated();

  /// Get the message text
  QString message() const;

  /// Set the message text
  void setMessage(const QString& message);

  /// Set the icon (scaled to 56x56)
  void setIcon(const QPixmap& icon);

  /// Update target position (handles in-progress animations)
  void updateTargetPosition(const QPoint& target);

  /// Check if the toast is currently animating in (not closing)
  bool isAnimatingIn() const;

signals:
  /// Emitted when the toast is fully closed (after animation)
  void closed();

protected:
  void showEvent(QShowEvent* event) override;

private slots:
  void onCloseClicked();
  void onAnimationFinished();
  void onTimeout();

private:
  void setupUI();
  void setupAnimation();
  QPixmap createRoundedPixmap(const QPixmap& source, int radius);

  QHBoxLayout* _layout;
  QLabel* _icon_label;
  QLabel* _message_label;
  QPushButton* _close_button;
  QPropertyAnimation* _slide_animation;
  QTimer* _timeout_timer;

  int _timeout_ms;
  bool _is_closing;

  static constexpr int BORDER_RADIUS = 6;
  static constexpr int ICON_SIZE = 128;
  static constexpr int ANIMATION_DURATION_MS = 300;
};

#endif  // TOAST_NOTIFICATION_H
