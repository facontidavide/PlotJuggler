/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef TOAST_MANAGER_H
#define TOAST_MANAGER_H

#include <QObject>
#include <QWidget>
#include <QList>
#include <QPixmap>

class ToastNotification;

class ToastManager : public QObject
{
  Q_OBJECT

public:
  explicit ToastManager(QWidget* parent_widget);
  ~ToastManager() override;

  /// Show a toast notification with optional icon
  /// @param message Text/HTML message to display
  /// @param icon Optional 56x56 icon
  /// @param timeout_ms Auto-dismiss timeout (default 8000ms)
  void showToast(const QString& message, const QPixmap& icon = QPixmap(), int timeout_ms = 8000);

  /// Update container position (call on parent resize)
  void updatePosition();

  /// Set margins from edge of parent widget
  void setMargins(int right, int bottom);

  /// Set spacing between stacked toasts
  void setSpacing(int spacing);

  /// Set maximum toast width
  void setMaxWidth(int width);

private slots:
  void onToastClosed();

private:
  void repositionToasts();

  QWidget* _parent_widget;
  QWidget* _container;
  QList<ToastNotification*> _toasts;

  int _margin_right;
  int _margin_bottom;
  int _spacing;
  int _max_width;
};

#endif  // TOAST_MANAGER_H
