/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include "PlotJuggler/plotdata.h"

namespace PJ
{

template <typename T>
inline bool isEqual(const T& a, const T& b)
{
  if constexpr (std::is_floating_point_v<T>)
  {
    return std::abs(a - b) <=
           std::numeric_limits<T>::epsilon() * std::max(std::abs(a), std::abs(b));
  }
  else
  {
    return a == b;
  }
}

class MonitoredValue : public QObject
{
  Q_OBJECT
public:
  MonitoredValue(QObject* parent = nullptr) : QObject(parent), _value(0)
  {
  }

  void set(double newValue)
  {
    double prev = _value;
    _value = newValue;
    if (fabs(newValue - prev) > std::numeric_limits<double>::epsilon())
    {
      emit valueChanged(_value);
    }
  }

  double get() const
  {
    return _value;
  }
signals:
  void valueChanged(double);

private:
  double _value;
};

struct MoveDataRet
{
  std::vector<std::string> added_curves;
  bool curves_updated = false;
  bool data_pushed = false;
};

MoveDataRet MoveData(PlotDataMapRef& source, PlotDataMapRef& destination, bool remove_older);
}  // namespace PJ

#endif  // UTILS_H
