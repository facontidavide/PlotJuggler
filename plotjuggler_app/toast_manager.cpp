/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "toast_manager.h"
#include "toast_notification.h"

#include <QLayout>

ToastManager::ToastManager(QWidget* parent_widget)
  : QObject(parent_widget)
  , _parent_widget(parent_widget)
  , _container(nullptr)
  , _margin_right(16)
  , _margin_bottom(16)
  , _spacing(8)
  , _max_width(400)
{
  // Create container as child of parent widget
  _container = new QWidget(_parent_widget);
  _container->setObjectName("toastManagerContainer");
  _container->setAttribute(Qt::WA_TransparentForMouseEvents, false);
  _container->setAttribute(Qt::WA_TranslucentBackground);
  _container->hide();
}

ToastManager::~ToastManager()
{
  // Toasts are children of container, will be deleted automatically
}

void ToastManager::showToast(const QString& message, const QPixmap& icon, int timeout_ms)
{
  ToastNotification* toast = new ToastNotification(message, icon, timeout_ms, _container);
  toast->setMaximumWidth(_max_width);

  _toasts.append(toast);

  connect(toast, &ToastNotification::closed, this, &ToastManager::onToastClosed);

  // Update container size and position
  updatePosition();

  // Show container
  _container->show();
  _container->raise();

  // Position all toasts
  repositionToasts();

  // Animate the new toast
  toast->showAnimated();
}

void ToastManager::updatePosition()
{
  if (!_parent_widget)
    return;

  int container_width = _max_width + _margin_right * 2;
  int container_height = _parent_widget->height();

  _container->setFixedSize(container_width, container_height);
  _container->move(_parent_widget->width() - container_width, 0);

  if (_container->isVisible())
  {
    repositionToasts();
  }
}

void ToastManager::setMargins(int right, int bottom)
{
  _margin_right = right;
  _margin_bottom = bottom;
  if (_container->isVisible())
  {
    updatePosition();
  }
}

void ToastManager::setSpacing(int spacing)
{
  _spacing = spacing;
  if (_container->isVisible())
  {
    repositionToasts();
  }
}

void ToastManager::setMaxWidth(int width)
{
  _max_width = width;
  for (auto* toast : _toasts)
  {
    toast->setMaximumWidth(width);
  }
  if (_container->isVisible())
  {
    updatePosition();
  }
}

void ToastManager::onToastClosed()
{
  ToastNotification* toast = qobject_cast<ToastNotification*>(sender());
  if (toast)
  {
    _toasts.removeOne(toast);
  }

  if (_toasts.isEmpty())
  {
    _container->hide();
  }
  else
  {
    repositionToasts();
  }
}

void ToastManager::repositionToasts()
{
  if (_toasts.isEmpty())
    return;

  const int container_width = _container->width();
  const int container_height = _container->height();
  const int toast_width = _max_width;
  const int x = container_width - toast_width - _margin_right;

  // Calculate positions for all toasts (newest = last in list = at bottom)
  // We iterate from bottom (last toast) to top (first toast)
  int current_y = container_height - _margin_bottom;

  for (int i = _toasts.size() - 1; i >= 0; --i)
  {
    ToastNotification* toast = _toasts[i];

    // Set width, then get the height for that width
    toast->setFixedWidth(toast_width);

    // Force layout to recalculate with new width
    toast->layout()->activate();

    // Get height that fits the content at this width
    int toast_height = toast->heightForWidth(toast_width);
    if (toast_height < 0)
    {
      // heightForWidth not implemented, use sizeHint
      toast_height = toast->sizeHint().height();
    }
    // Ensure minimum height
    toast_height = qMax(toast_height, toast->minimumHeight());

    toast->setFixedHeight(toast_height);

    // Position this toast above the current y
    current_y -= toast_height;
    QPoint targetPos(x, current_y);
    toast->updateTargetPosition(targetPos);
    toast->show();

    // Add spacing for next toast above
    current_y -= _spacing;
  }
}
