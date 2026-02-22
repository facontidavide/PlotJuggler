/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_TIMESERIES_H
#define PJ_TIMESERIES_H

#include "plotdatabase.h"
#include <algorithm>
#include <numeric>

namespace PJ
{
template <typename Value>
class TimeseriesBase : public PlotDataBase<double, Value>
{
protected:
  double _max_range_x;

public:
  using Point = typename PlotDataBase<double, Value>::Point;
  using ConstIterator = typename PlotDataBase<double, Value>::ConstIterator;

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
    if (index < 0)
    {
      return std::nullopt;
    }
    return std::optional(this->_values.valueAt(index));
  }

  void pushBack(const Point& p) override
  {
    auto temp = p;
    pushBack(std::move(temp));
  }

  void pushBack(Point&& p) override
  {
    bool need_sorting = (!this->_timestamps.empty() && p.x < this->back().x);

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

  virtual void pushUnsorted(const Point& p)
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
      this->_timestamps.push_back(p.x);
      this->_values.push_back(p.y);
    }
  }

  void sort()
  {
    size_t n = this->_timestamps.size();
    if (n == 0)
    {
      return;
    }

    // Build index array and sort by timestamp
    std::vector<size_t> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [this](size_t a, size_t b) { return this->_timestamps[a] < this->_timestamps[b]; });

    // Flatten timestamps and values, then rebuild in sorted order
    std::vector<double> sorted_ts(n);
    std::vector<Value> sorted_vals(n);
    for (size_t i = 0; i < n; i++)
    {
      sorted_ts[i] = this->_timestamps[indices[i]];
      sorted_vals[i] = this->_values.valueAt(indices[i]);
    }

    this->_timestamps.clear();
    this->_values.clear();
    for (size_t i = 0; i < n; i++)
    {
      this->_timestamps.push_back(sorted_ts[i]);
      this->_values.push_back(std::move(sorted_vals[i]));
    }

    // Recompute ranges
    Range range_x;
    range_x.min = std::numeric_limits<double>::max();
    range_x.max = std::numeric_limits<double>::lowest();
    Range range_y;
    range_y.min = std::numeric_limits<double>::max();
    range_y.max = std::numeric_limits<double>::lowest();

    for (size_t i = 0; i < n; i++)
    {
      double x = this->_timestamps[i];
      range_x.min = std::min(range_x.min, x);
      range_x.max = std::max(range_x.max, x);

      if constexpr (std::is_arithmetic_v<Value>)
      {
        double y = static_cast<double>(this->_values.valueAt(i));
        range_y.min = std::min(range_y.min, y);
        range_y.max = std::max(range_y.max, y);
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
    if (_max_range_x < std::numeric_limits<double>::max() && !this->_timestamps.empty())
    {
      auto const back_x = this->_timestamps.back();
      while (this->_timestamps.size() > 2 && (back_x - this->_timestamps.front()) > _max_range_x)
      {
        this->popFront();
      }
    }
  }
};

//--------------------

template <typename Value>
inline int TimeseriesBase<Value>::getIndexFromX(double x) const
{
  if (this->_timestamps.empty())
  {
    return -1;
  }
  auto lower = std::lower_bound(this->_timestamps.begin(), this->_timestamps.end(), x);
  auto index = std::distance(this->_timestamps.begin(), lower);

  if (index >= static_cast<decltype(index)>(this->_timestamps.size()))
  {
    return this->_timestamps.size() - 1;
  }
  if (index < 0)
  {
    return 0;
  }

  if (index > 0 &&
      (std::abs(this->_timestamps[index - 1] - x) < std::abs(this->_timestamps[index] - x)))
  {
    index = index - 1;
  }
  return index;
}

}  // namespace PJ

#endif
