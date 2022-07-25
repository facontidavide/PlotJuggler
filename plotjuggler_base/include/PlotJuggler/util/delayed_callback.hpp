/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_DELAYED_CALLBACK_HPP
#define PJ_DELAYED_CALLBACK_HPP

#include <QTimer>
#include <QMetaMethod>
#include <functional>

namespace PJ
{
// Simple utility to avoid triggering a certain slot too often
class DelayedCallback
{
public:
  DelayedCallback()
  {
    _delay_timer = new QTimer();
    _delay_timer->setSingleShot(true);
  }

  ~DelayedCallback()
  {
    delete _delay_timer;
  }

  template <class Function>
  void connectCallback(Function callback)
  {
    QObject::connect(_delay_timer, &QTimer::timeout, callback);
  }

  void triggerSignal(int delay_ms)
  {
    if (!_delay_timer->isActive())
    {
      _delay_timer->start(delay_ms);
    }
  }

private:
  QTimer* _delay_timer;
};

}  // namespace PJ

#endif  // PJ_DELAYED_CALLBACK_HPP
