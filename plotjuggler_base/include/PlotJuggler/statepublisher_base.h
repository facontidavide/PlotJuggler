/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef STATEPUBLISHER_TEMPLATE_H
#define STATEPUBLISHER_TEMPLATE_H

#include <QObject>
#include <QtPlugin>
#include <QMenu>
#include <QDomElement>
#include <functional>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"

namespace PJ
{
class StatePublisher : public PlotJugglerPlugin
{
  Q_OBJECT

public:
  /// True if started
  virtual bool enabled() const = 0;

  /// Method called when the timetracker is moved manually to a certain position.
  virtual void updateState(double current_time) = 0;

  /// Method called when the "play" button is checked.
  /// @param interval is seconds passed since the last time play was called.
  virtual void play(double interval) = 0;

  virtual ~StatePublisher() = default;

  void setDataMap(const PlotDataMapRef* datamap)
  {
    _datamap = datamap;
  }

public slots:
  /// Method called when the checkbox "enabled" is checked in the main app.
  virtual void setEnabled(bool enabled) = 0;

signals:

  /// signal to be emitted when the plugin disable itself.
  void closed();

protected:
  const PlotDataMapRef* _datamap;
};

using StatePublisherPtr = std::shared_ptr<StatePublisher>;

}  // namespace PJ

QT_BEGIN_NAMESPACE
#define StatePublisher_iid "facontidavide.PlotJuggler3.StatePublisher"
Q_DECLARE_INTERFACE(PJ::StatePublisher, StatePublisher_iid)
QT_END_NAMESPACE

#endif
