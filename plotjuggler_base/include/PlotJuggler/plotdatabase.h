/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_PLOTDATA_BASE_H
#define PJ_PLOTDATA_BASE_H

#include <memory>
#include <string>
#include <deque>
#include <type_traits>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <optional>
#include <iterator>

#include <QVariant>
#include <QtGlobal>

#include "chunked_values.h"

namespace PJ
{
struct Range
{
  double min = std::numeric_limits<double>::lowest();
  double max = std::numeric_limits<double>::max();
};

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
const auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
const auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

typedef std::optional<Range> RangeOpt;

// Attributes supported by the GUI.
enum PlotAttribute
{
  // color to be displayed on the curve list.
  // Type: QColor
  TEXT_COLOR,

  // font style to be displayed on the curve list.
  // Type: boolean. Default: false
  ITALIC_FONTS,

  // Tooltip to be displayed on the curve list.
  // Type: QString
  TOOL_TIP,

  // Color of the curve in the plot.
  // Type: QColor
  COLOR_HINT
};

using Attributes = std::unordered_map<PlotAttribute, QVariant>;

inline bool CheckType(PlotAttribute attr, const QVariant& value)
{
  switch (attr)
  {
    case TEXT_COLOR:
    case COLOR_HINT:
      return value.type() == QVariant::Color;
    case ITALIC_FONTS:
      return value.type() == QVariant::Bool;
    case TOOL_TIP:
      return value.type() == QVariant::String;
  }
  return false;
}

/**
 * @brief PlotData may or may not have a group. Think of PlotGroup
 * as a way to say that certain set of series are "siblings".
 *
 */
class PlotGroup
{
public:
  using Ptr = std::shared_ptr<PlotGroup>;

  PlotGroup(const std::string& name) : _name(name)
  {
  }

  const std::string& name() const
  {
    return _name;
  }

  const Attributes& attributes() const
  {
    return _attributes;
  }

  Attributes& attributes()
  {
    return _attributes;
  }

  void setAttribute(const PlotAttribute& id, const QVariant& value)
  {
    _attributes[id] = value;
  }

  QVariant attribute(const PlotAttribute& id) const
  {
    auto it = _attributes.find(id);
    return (it == _attributes.end()) ? QVariant() : it->second;
  }

private:
  const std::string _name;
  Attributes _attributes;
};

// A Generic series of points
template <typename TypeX, typename Value>
class PlotDataBase
{
public:
  class Point
  {
  public:
    TypeX x;
    Value y;
    Point(TypeX _x, Value _y) : x(_x), y(_y)
    {
    }
    Point() = default;
  };

  enum
  {
    MAX_CAPACITY = 1024 * 1024,
    ASYNC_BUFFER_CAPACITY = 1024
  };

  class ConstIterator
  {
    const PlotDataBase* _owner;
    size_t _index;
    mutable Point _cached;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = Point;
    using difference_type = std::ptrdiff_t;
    using reference = const Point&;
    using pointer = const Point*;

    ConstIterator() : _owner(nullptr), _index(0)
    {
    }
    ConstIterator(const PlotDataBase* owner, size_t index) : _owner(owner), _index(index)
    {
    }

    const Point& operator*() const
    {
      _cached.x = _owner->_timestamps[_index];
      _cached.y = _owner->_values.valueAt(_index);
      return _cached;
    }

    const Point* operator->() const
    {
      operator*();
      return &_cached;
    }

    const Point& operator[](difference_type n) const
    {
      _cached.x = _owner->_timestamps[_index + n];
      _cached.y = _owner->_values.valueAt(_index + n);
      return _cached;
    }

    ConstIterator& operator++()
    {
      ++_index;
      return *this;
    }
    ConstIterator operator++(int)
    {
      auto tmp = *this;
      ++_index;
      return tmp;
    }
    ConstIterator& operator--()
    {
      --_index;
      return *this;
    }
    ConstIterator operator--(int)
    {
      auto tmp = *this;
      --_index;
      return tmp;
    }

    ConstIterator& operator+=(difference_type n)
    {
      _index += n;
      return *this;
    }
    ConstIterator& operator-=(difference_type n)
    {
      _index -= n;
      return *this;
    }
    ConstIterator operator+(difference_type n) const
    {
      return { _owner, _index + n };
    }
    ConstIterator operator-(difference_type n) const
    {
      return { _owner, _index - n };
    }
    difference_type operator-(const ConstIterator& other) const
    {
      return static_cast<difference_type>(_index) - static_cast<difference_type>(other._index);
    }

    bool operator==(const ConstIterator& other) const
    {
      return _index == other._index;
    }
    bool operator!=(const ConstIterator& other) const
    {
      return _index != other._index;
    }
    bool operator<(const ConstIterator& other) const
    {
      return _index < other._index;
    }
    bool operator>(const ConstIterator& other) const
    {
      return _index > other._index;
    }
    bool operator<=(const ConstIterator& other) const
    {
      return _index <= other._index;
    }
    bool operator>=(const ConstIterator& other) const
    {
      return _index >= other._index;
    }

    friend ConstIterator operator+(difference_type n, const ConstIterator& it)
    {
      return it + n;
    }

    size_t iterIndex() const
    {
      return _index;
    }
  };

  typedef ConstIterator Iterator;
  typedef Value ValueT;

  PlotDataBase(const std::string& name, PlotGroup::Ptr group)
    : _name(name), _range_x_dirty(true), _range_y_dirty(true), _group(group)
  {
  }

  PlotDataBase(const PlotDataBase& other) = delete;
  PlotDataBase(PlotDataBase&& other) = default;

  PlotDataBase& operator=(const PlotDataBase& other) = delete;
  PlotDataBase& operator=(PlotDataBase&& other) = default;

  void clonePoints(const PlotDataBase& other)
  {
    other.flushTempPoint();
    _timestamps = other._timestamps;
    _values = other._values;
    _range_x = other._range_x;
    _range_y = other._range_y;
    _range_x_dirty = other._range_x_dirty;
    _range_y_dirty = other._range_y_dirty;
    _temp_point_index = -1;
  }

  void clonePoints(PlotDataBase&& other)
  {
    other.flushTempPoint();
    _timestamps = std::move(other._timestamps);
    _values = std::move(other._values);
    _range_x = other._range_x;
    _range_y = other._range_y;
    _range_x_dirty = other._range_x_dirty;
    _range_y_dirty = other._range_y_dirty;
    _temp_point_index = -1;
  }

  virtual ~PlotDataBase() = default;

  const std::string& plotName() const
  {
    return _name;
  }

  const PlotGroup::Ptr& group() const
  {
    return _group;
  }

  void changeGroup(PlotGroup::Ptr group)
  {
    _group = group;
  }

  virtual size_t size() const
  {
    return _timestamps.size();
  }

  bool empty() const
  {
    return _timestamps.empty();
  }

  virtual bool isTimeseries() const
  {
    return false;
  }

  const Point& at(size_t index) const
  {
    flushTempPoint();
    _temp_point.x = _timestamps[index];
    _temp_point.y = _values.valueAt(index);
    _temp_point_index = -1;
    return _temp_point;
  }

  Point& at(size_t index)
  {
    flushTempPoint();
    _temp_point.x = _timestamps[index];
    _temp_point.y = _values.valueAt(index);
    _temp_point_index = static_cast<ssize_t>(index);
    return _temp_point;
  }

  const Point& operator[](size_t index) const
  {
    return at(index);
  }

  Point& operator[](size_t index)
  {
    return at(index);
  }

  virtual void clear()
  {
    flushTempPoint();
    _timestamps.clear();
    _values.clear();
    _range_x_dirty = true;
    _range_y_dirty = true;
  }

  const Attributes& attributes() const
  {
    return _attributes;
  }

  Attributes& attributes()
  {
    return _attributes;
  }

  void setAttribute(PlotAttribute id, const QVariant& value)
  {
    _attributes[id] = value;
    if (!CheckType(id, value))
    {
      throw std::runtime_error("PlotDataBase::setAttribute : wrong type");
    }
  }

  QVariant attribute(PlotAttribute id) const
  {
    auto it = _attributes.find(id);
    return (it == _attributes.end()) ? QVariant() : it->second;
  }

  const Point& front() const
  {
    flushTempPoint();
    _temp_point.x = _timestamps.front();
    _temp_point.y = _values.valueAt(0);
    _temp_point_index = -1;
    return _temp_point;
  }

  const Point& back() const
  {
    flushTempPoint();
    _temp_point.x = _timestamps.back();
    _temp_point.y = _values.valueAt(_values.size() - 1);
    _temp_point_index = -1;
    return _temp_point;
  }

  ConstIterator begin() const
  {
    return ConstIterator(this, 0);
  }

  ConstIterator end() const
  {
    return ConstIterator(this, _timestamps.size());
  }

  Iterator begin()
  {
    return Iterator(this, 0);
  }

  Iterator end()
  {
    return Iterator(this, _timestamps.size());
  }

  // template specialization for types that support compare operator
  virtual RangeOpt rangeX() const
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (_timestamps.empty())
      {
        return std::nullopt;
      }
      if (_range_x_dirty)
      {
        _range_x.min = _timestamps.front();
        _range_x.max = _range_x.min;
        for (double x : _timestamps)
        {
          _range_x.min = std::min(_range_x.min, x);
          _range_x.max = std::max(_range_x.max, x);
        }
        _range_x_dirty = false;
      }
      return _range_x;
    }
    return std::nullopt;
  }

  // Optimized: scans chunk metadata instead of every point
  virtual RangeOpt rangeY() const
  {
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (_timestamps.empty())
      {
        return std::nullopt;
      }
      if (_range_y_dirty)
      {
        flushTempPoint();
        _range_y.min = std::numeric_limits<double>::max();
        _range_y.max = std::numeric_limits<double>::lowest();
        for (const auto& chunk : _values.chunks())
        {
          if (chunk.count > 0)
          {
            _range_y.min = std::min(_range_y.min, chunk.min_y);
            _range_y.max = std::max(_range_y.max, chunk.max_y);
          }
        }
        _range_y_dirty = false;
      }
      return _range_y;
    }
    return std::nullopt;
  }

  virtual void pushBack(const Point& p)
  {
    auto temp = p;
    pushBack(std::move(temp));
  }

  virtual void pushBack(Point&& p)
  {
    flushTempPoint();
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (std::isinf(p.x) || std::isnan(p.x))
      {
        return;  // skip
      }
      pushUpdateRangeX(p);
    }
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (std::isinf(p.y) || std::isnan(p.y))
      {
        return;  // skip
      }
      pushUpdateRangeY(p);
    }

    _timestamps.push_back(p.x);
    _values.push_back(std::move(p.y));
  }

  virtual void insert(ConstIterator it, Point&& p)
  {
    flushTempPoint();
    size_t index = it.iterIndex();
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (std::isinf(p.x) || std::isnan(p.x))
      {
        return;  // skip
      }
      pushUpdateRangeX(p);
    }
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (std::isinf(p.y) || std::isnan(p.y))
      {
        return;  // skip
      }
      pushUpdateRangeY(p);
    }

    _timestamps.insert(_timestamps.begin() + index, p.x);
    _values.insert(index, std::move(p.y));
  }

  virtual void popFront()
  {
    flushTempPoint();
    const double x = _timestamps.front();
    const auto y = _values.valueAt(0);

    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (!_range_x_dirty && (x == _range_x.max || x == _range_x.min))
      {
        _range_x_dirty = true;
      }
    }

    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (!_range_y_dirty && (y == _range_y.max || y == _range_y.min))
      {
        _range_y_dirty = true;
      }
    }
    _timestamps.pop_front();
    _values.pop_front();
  }

  const std::deque<double>& timestamps() const
  {
    return _timestamps;
  }

  const ChunkedValues<Value>& values() const
  {
    return _values;
  }

protected:
  std::string _name;
  Attributes _attributes;
  std::deque<double> _timestamps;
  mutable ChunkedValues<Value> _values;

  mutable Point _temp_point;
  mutable ssize_t _temp_point_index = -1;

  mutable Range _range_x;
  mutable Range _range_y;
  mutable bool _range_x_dirty;
  mutable bool _range_y_dirty;
  mutable std::shared_ptr<PlotGroup> _group;

  void flushTempPoint() const
  {
    if (_temp_point_index >= 0)
    {
      _values.writeBack(static_cast<size_t>(_temp_point_index), _temp_point.y);
      _temp_point_index = -1;
    }
  }

  // template specialization for types that support compare operator
  virtual void pushUpdateRangeX(const Point& p)
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (_timestamps.empty())
      {
        _range_x_dirty = false;
        _range_x.min = p.x;
        _range_x.max = p.x;
      }
      if (!_range_x_dirty)
      {
        if (p.x > _range_x.max)
        {
          _range_x.max = p.x;
        }
        else if (p.x < _range_x.min)
        {
          _range_x.min = p.x;
        }
        else
        {
          _range_x_dirty = true;
        }
      }
    }
  }

  // template specialization for types that support compare operator
  virtual void pushUpdateRangeY(const Point& p)
  {
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (!_range_y_dirty)
      {
        if (p.y > _range_y.max)
        {
          _range_y.max = p.y;
        }
        else if (p.y < _range_y.min)
        {
          _range_y.min = p.y;
        }
        else
        {
          _range_y_dirty = true;
        }
      }
    }
  }
};

}  // namespace PJ

#endif
