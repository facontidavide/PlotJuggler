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

#include <QVariant>
#include <QtGlobal>

namespace PJ
{
struct Range
{
  double min;
  double max;
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

  class Iterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Point;
    using pointer = Point*;
    using reference = Point&;

    Iterator(typename std::deque<TypeX>::iterator x_it,
             typename std::deque<Value>::iterator y_it)
      : _x_it(x_it), _y_it(y_it)
    {
    }

    Point operator*()
    {
      return Point(*_x_it, *_y_it);
    }
    Iterator& operator++()
    {
      ++_x_it;
      ++_y_it;
      return *this;
    }
    Iterator operator++(int)
    {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    Iterator& operator--()
    {
      --_x_it;
      --_y_it;
      return *this;
    }
    Iterator operator--(int)
    {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }
    Iterator& operator+=(difference_type n)
    {
      _x_it += n;
      _y_it += n;
      return *this;
    }
    Iterator operator+(difference_type n) const
    {
      return Iterator(_x_it + n, _y_it + n);
    }
    Iterator& operator-=(difference_type n)
    {
      _x_it -= n;
      _y_it -= n;
      return *this;
    }
    Iterator operator-(difference_type n) const
    {
      return Iterator(_x_it - n, _y_it - n);
    }
    difference_type operator-(const Iterator& other) const
    {
      return _x_it - other._x_it;
    }
    Point operator[](difference_type n)
    {
      return Point(*(_x_it + n), *(_y_it + n));
    }
    bool operator==(const Iterator& other) const
    {
      return _x_it == other._x_it;
    }
    bool operator!=(const Iterator& other) const
    {
      return _x_it != other._x_it;
    }
    bool operator<(const Iterator& other) const
    {
      return _x_it < other._x_it;
    }
    bool operator<=(const Iterator& other) const
    {
      return _x_it <= other._x_it;
    }
    bool operator>(const Iterator& other) const
    {
      return _x_it > other._x_it;
    }
    bool operator>=(const Iterator& other) const
    {
      return _x_it >= other._x_it;
    }

  private:
    typename std::deque<TypeX>::iterator _x_it;
    typename std::deque<Value>::iterator _y_it;
  };

  class ConstIterator
  {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Point;
    using pointer = const Point*;
    using reference = const Point&;

    ConstIterator(typename std::deque<TypeX>::const_iterator x_it,
                  typename std::deque<Value>::const_iterator y_it)
      : _x_it(x_it), _y_it(y_it)
    {
    }

    Point operator*() const
    {
      return Point(*_x_it, *_y_it);
    }
    ConstIterator& operator++()
    {
      ++_x_it;
      ++_y_it;
      return *this;
    }
    ConstIterator operator++(int)
    {
      ConstIterator tmp = *this;
      ++(*this);
      return tmp;
    }
    ConstIterator& operator--()
    {
      --_x_it;
      --_y_it;
      return *this;
    }
    ConstIterator operator--(int)
    {
      ConstIterator tmp = *this;
      --(*this);
      return tmp;
    }
    ConstIterator& operator+=(difference_type n)
    {
      _x_it += n;
      _y_it += n;
      return *this;
    }
    ConstIterator operator+(difference_type n) const
    {
      return ConstIterator(_x_it + n, _y_it + n);
    }
    ConstIterator& operator-=(difference_type n)
    {
      _x_it -= n;
      _y_it -= n;
      return *this;
    }
    ConstIterator operator-(difference_type n) const
    {
      return ConstIterator(_x_it - n, _y_it - n);
    }
    difference_type operator-(const ConstIterator& other) const
    {
      return _x_it - other._x_it;
    }
    Point operator[](difference_type n) const
    {
      return Point(*(_x_it + n), *(_y_it + n));
    }
    bool operator==(const ConstIterator& other) const
    {
      return _x_it == other._x_it;
    }
    bool operator!=(const ConstIterator& other) const
    {
      return _x_it != other._x_it;
    }
    bool operator<(const ConstIterator& other) const
    {
      return _x_it < other._x_it;
    }
    bool operator<=(const ConstIterator& other) const
    {
      return _x_it <= other._x_it;
    }
    bool operator>(const ConstIterator& other) const
    {
      return _x_it > other._x_it;
    }
    bool operator>=(const ConstIterator& other) const
    {
      return _x_it >= other._x_it;
    }

  private:
    typename std::deque<TypeX>::const_iterator _x_it;
    typename std::deque<Value>::const_iterator _y_it;
  };
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
    _x_data = other._x_data;
    _y_data = other._y_data;
    _range_x = other._range_x;
    _range_y = other._range_y;
    _range_x_dirty = other._range_x_dirty;
    _range_y_dirty = other._range_y_dirty;
  }

  void clonePoints(PlotDataBase&& other)
  {
    _x_data = std::move(other._x_data);
    _y_data = std::move(other._y_data);
    _range_x = other._range_x;
    _range_y = other._range_y;
    _range_x_dirty = other._range_x_dirty;
    _range_y_dirty = other._range_y_dirty;
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
    return _x_data.size();
  }

  virtual bool isTimeseries() const
  {
    return false;
  }

  Point at(size_t index) const
  {
    return Point(_x_data[index], _y_data[index]);
  }

  Point at(size_t index)
  {
    return Point(_x_data[index], _y_data[index]);
  }

  void setPoint(size_t index, const Point& p)
  {
    _x_data[index] = p.x;
    _y_data[index] = p.y;
    _range_x_dirty = true;
    _range_y_dirty = true;
  }

  Point operator[](size_t index) const
  {
    return at(index);
  }

  Point operator[](size_t index)
  {
    return at(index);
  }

  virtual void clear()
  {
    _x_data.clear();
    _y_data.clear();
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

  Point front() const
  {
    return Point(_x_data.front(), _y_data.front());
  }

  Point back() const
  {
    return Point(_x_data.back(), _y_data.back());
  }

  ConstIterator begin() const
  {
    return ConstIterator(_x_data.begin(), _y_data.begin());
  }

  ConstIterator end() const
  {
    return ConstIterator(_x_data.end(), _y_data.end());
  }

  Iterator begin()
  {
    return Iterator(_x_data.begin(), _y_data.begin());
  }

  Iterator end()
  {
    return Iterator(_x_data.end(), _y_data.end());
  }

  // template specialization for types that support compare operator
  virtual RangeOpt rangeX() const
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (_x_data.empty())
      {
        return std::nullopt;
      }
      if (_range_x_dirty)
      {
        _range_x.min = _x_data.front();
        _range_x.max = _range_x.min;
        for (const auto& x : _x_data)
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

  // template specialization for types that support compare operator
  virtual RangeOpt rangeY() const
  {
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (_y_data.empty())
      {
        return std::nullopt;
      }
      if (_range_y_dirty)
      {
        _range_y.min = _y_data.front();
        _range_y.max = _range_y.min;
        for (const auto& y : _y_data)
        {
          _range_y.min = std::min(_range_y.min, y);
          _range_y.max = std::max(_range_y.max, y);
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
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (std::isinf(p.x) || std::isnan(p.x))
      {
        return;  // skip
      }
      pushUpdateRangeX(p.x);
    }
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (std::isinf(p.y) || std::isnan(p.y))
      {
        return;  // skip
      }
      pushUpdateRangeY(p.y);
    }

    _x_data.emplace_back(std::move(p.x));
    _y_data.emplace_back(std::move(p.y));
  }

  virtual void insert(Iterator it, Point&& p)
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (std::isinf(p.x) || std::isnan(p.x))
      {
        return;  // skip
      }
      pushUpdateRangeX(p.x);
    }
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (std::isinf(p.y) || std::isnan(p.y))
      {
        return;  // skip
      }
      pushUpdateRangeY(p.y);
    }

    size_t index = it - begin();
    auto x_it = _x_data.begin() + index;
    auto y_it = _y_data.begin() + index;
    _x_data.insert(x_it, std::move(p.x));
    _y_data.insert(y_it, std::move(p.y));
  }

  virtual void popFront()
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (!_range_x_dirty &&
          (_x_data.front() == _range_x.max || _x_data.front() == _range_x.min))
      {
        _range_x_dirty = true;
      }
    }

    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (!_range_y_dirty &&
          (_y_data.front() == _range_y.max || _y_data.front() == _range_y.min))
      {
        _range_y_dirty = true;
      }
    }
    _x_data.pop_front();
    _y_data.pop_front();
  }

protected:
  std::string _name;
  Attributes _attributes;
  std::deque<TypeX> _x_data;
  std::deque<Value> _y_data;

  mutable Range _range_x;
  mutable Range _range_y;
  mutable bool _range_x_dirty;
  mutable bool _range_y_dirty;
  mutable std::shared_ptr<PlotGroup> _group;

  // template specialization for types that support compare operator
  virtual void pushUpdateRangeX(const TypeX& x)
  {
    if constexpr (std::is_arithmetic_v<TypeX>)
    {
      if (_x_data.empty())
      {
        _range_x_dirty = false;
        _range_x.min = x;
        _range_x.max = x;
      }
      if (!_range_x_dirty)
      {
        if (x > _range_x.max)
        {
          _range_x.max = x;
        }
        else if (x < _range_x.min)
        {
          _range_x.min = x;
        }
        else
        {
          _range_x_dirty = true;
        }
      }
    }
  }

  // template specialization for types that support compare operator
  virtual void pushUpdateRangeY(const Value& y)
  {
    if constexpr (std::is_arithmetic_v<Value>)
    {
      if (!_range_y_dirty)
      {
        if (y > _range_y.max)
        {
          _range_y.max = y;
        }
        else if (y < _range_y.min)
        {
          _range_y.min = y;
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
