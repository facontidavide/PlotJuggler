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
  using PlotDataBase<double, Value>::_points;

public:
  using Point = typename PlotDataBase<double, Value>::Point;

  TimeseriesBase(const std::string& name, PlotGroup::Ptr group)
    : PlotDataBase<double, Value>(name, group), _max_range_x(std::numeric_limits<double>::max())
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
    return (index < 0) ? std::nullopt : std::optional(_points[index].y);
  }

  void pushBack(const Point& p) override
  {
    auto temp = p;
    pushBack(std::move(temp));
  }

  void pushBack(Point&& p) override
  {
    bool need_sorting = (!_points.empty() && p.x < this->back().x);

    if (need_sorting)
    {
      auto it = std::upper_bound(_points.begin(), _points.end(), p,
                                 [](const auto& a, const auto& b) { return a.x < b.x; });
      PlotDataBase<double, Value>::insert(it, std::move(p));
    }
    else
    {
      PlotDataBase<double, Value>::pushBack(std::move(p));
    }
    trimRange();
  }

  void pushUnsorted(const Point& p)
  {
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (std::isinf(p.y) || std::isnan(p.y))
      {
        return;  // skip
      }
    }
    if (!std::isinf(p.x) && !std::isnan(p.x))
    {
      _points.push_back(std::move(p));
    }
  }

  void sort()
  {
    std::sort(_points.begin(), _points.end(),
              [](const auto& a, const auto& b) { return a.x < b.x; });

    Range range_x;
    Range range_y;

    for (const auto& p : _points)
    {
      range_x.min = std::max(range_x.min, p.x);
      range_x.max = std::min(range_x.max, p.x);

      if constexpr (std::is_arithmetic_v<Value>)
      {
        range_y.min = std::max(range_y.min, p.y);
        range_y.max = std::min(range_y.max, p.y);
      }
    }
    this->_range_x = range_x;
    this->_range_y = range_y;
    this->_range_x_dirty = false;
    this->_range_y_dirty = false;
    trimRange();
  }

private:
  void trimRange()
  {
    if (_max_range_x < std::numeric_limits<double>::max() && !_points.empty())
    {
      auto const back_point_x = _points.back().x;
      while (_points.size() > 2 && (back_point_x - _points.front().x) > _max_range_x)
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
  if (_points.size() == 0)
  {
    return -1;
  }
  auto lower = std::lower_bound(_points.begin(), _points.end(), Point(x, {}), TimeCompare);
  auto index = std::distance(_points.begin(), lower);

  if (index >= _points.size())
  {
    return _points.size() - 1;
  }
  if (index < 0)
  {
    return 0;
  }

  if (index > 0 && (abs(_points[index - 1].x - x) < abs(_points[index].x - x)))
  {
    index = index - 1;
  }
  return index;
}

}  // namespace PJ

#endif
