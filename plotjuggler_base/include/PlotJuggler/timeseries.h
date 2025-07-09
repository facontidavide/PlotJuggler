/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_TIMESERIES_H
#define PJ_TIMESERIES_H

#include "plotdatabase.h"
#include <algorithm>

namespace PJ
{
template <typename Value>
class TimeseriesBase : public PlotDataBase<double, Value>
{
protected:
  double _max_range_x;
  using PlotDataBase<double, Value>::_x_data;
  using PlotDataBase<double, Value>::_y_data;

public:
  using Point = typename PlotDataBase<double, Value>::Point;

  TimeseriesBase(const std::string& name, PlotGroup::Ptr group)
    : PlotDataBase<double, Value>(name, group)
    , _max_range_x(std::numeric_limits<double>::max())
  {
  }

  TimeseriesBase(const TimeseriesBase& other) = delete;
  TimeseriesBase(TimeseriesBase&& other) = default;

  TimeseriesBase& operator=(const TimeseriesBase& other) = delete;
  TimeseriesBase& operator=(TimeseriesBase&& other) = default;

  virtual bool isTimeseries() const override
  {
    return true;
  }

  void setMaximumRangeX(double max_range)
  {
    _max_range_x = max_range;
    trimRange();
  }

  double maximumRangeX() const
  {
    return _max_range_x;
  }

  int getIndexFromX(double x) const;

  std::optional<Value> getYfromX(double x) const
  {
    int index = getIndexFromX(x);
    if (index < 0)
    {
      return std::nullopt;
    }

    // Handle both storage modes like at() method
    if (this->_const_y_value.has_value())
    {
      // Constant mode: return the constant value
      return std::optional(*this->_const_y_value);
    }
    else
    {
      // Variable mode: return the specific value
      return std::optional(this->_y_data[index]);
    }
  }

  void pushBack(const Point& p) override
  {
    auto temp = p;
    pushBack(std::move(temp));
  }

  void pushBack(Point&& p) override
  {
    bool need_sorting = (!_x_data.empty() && p.x < _x_data.back());

    if (need_sorting)
    {
      auto it = std::upper_bound(this->begin(), this->end(), p,
                                 [](const auto& a, const auto& b) { return a.x < b.x; });
      PlotDataBase<double, Value>::insert(it, std::move(p));
    }
    else
    {
      PlotDataBase<double, Value>::pushBack(std::move(p));
    }
    trimRange();
  }

private:
  void trimRange()
  {
    if (_max_range_x < std::numeric_limits<double>::max() && !_x_data.empty())
    {
      auto const back_point_x = _x_data.back();
      while (_x_data.size() > 2 && (back_point_x - _x_data.front()) > _max_range_x)
      {
        this->popFront();
      }
    }
  }

  static bool TimeCompare(const Point& a, const Point& b)
  {
    return a.x < b.x;
  }
};

//--------------------

template <typename Value>
inline int TimeseriesBase<Value>::getIndexFromX(double x) const
{
  if (_x_data.size() == 0)
  {
    return -1;
  }
  auto lower = std::lower_bound(_x_data.begin(), _x_data.end(), x);
  auto index = std::distance(_x_data.begin(), lower);

  if (index >= _x_data.size())
  {
    return _x_data.size() - 1;
  }
  if (index < 0)
  {
    return 0;
  }

  if (index > 0 && (abs(_x_data[index - 1] - x) < abs(_x_data[index] - x)))
  {
    index = index - 1;
  }
  return index;
}

}  // namespace PJ

#endif
