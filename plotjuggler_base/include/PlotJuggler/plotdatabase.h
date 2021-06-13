#ifndef PJ_PLOTDATA_BASE_H
#define PJ_PLOTDATA_BASE_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <type_traits>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <set>
#include <string_view>
#include <any>
#include <optional>
#include <QVariant>

namespace PJ {

struct Range
{
    double min;
    double max;
};

typedef std::optional<Range> RangeOpt;
using Attributes = std::map<std::string, QVariant>;

/**
 * @brief PlotData may or may not have a group. Think of PlotGroup
 * as a way to say that certain set of series are "siblings".
 *
 */
class PlotGroup
{
public:

  using Ptr = std::shared_ptr<PlotGroup>;

  PlotGroup(const std::string& name):
    _name( name )
  { }

  const std::string& name() const {
    return _name;
  }

  void setAttribute(const std::string& name, const QVariant& value)
  {
      _attributes[ name ] = value;
  }

  const Attributes& attributes() const{
    return _attributes;
  }

  Attributes& attributes() {
    return _attributes;
  }

  QVariant attribute(const std::string& name) const
  {
      auto it = _attributes.find( name );

      if( it ==  _attributes.end() ) {
        return {};
      }
      else {
          return it->second;
      }
  }
private:
  const std::string _name;
  Attributes _attributes;
};

// A Generic series of points
template <typename TypeX, typename Value >
class PlotDataBase
{

public:

    class Point
    {
    public:
        TypeX x;
        Value y;
        Point(TypeX _x, Value _y) : x(_x), y(_y)
        { }
        Point() = default;
    };

    enum
    {
        MAX_CAPACITY = 1024 * 1024,
        ASYNC_BUFFER_CAPACITY = 1024
    };

    typedef typename std::deque<Point>::iterator Iterator;
    typedef typename std::deque<Point>::const_iterator ConstIterator;

    PlotDataBase(const std::string& name, PlotGroup::Ptr group ):
        _name(name),
        _range_x_dirty(true),
        _range_y_dirty(true),
        _group(group)
    {}

    PlotDataBase(const PlotDataBase& other) = delete;
    PlotDataBase(PlotDataBase&& other) = default;

    PlotDataBase& operator=(const PlotDataBase& other) = delete;
    PlotDataBase& operator=(PlotDataBase&& other) = default;

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
        return _points.size();
    }

    const Point& at(size_t index) const
    {
        return _points[index];
    }

    Point& at(size_t index)
    {
        return _points[index];
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
        _points.clear();
        _range_x_dirty = true;
        _range_y_dirty = true;
    }

    void setAttribute(const std::string& name, const QVariant& value)
    {
        _attributes[ name ] = value;
    }

    const Attributes& attributes() const{
      return _attributes;
    }

    Attributes& attributes() {
      return _attributes;
    }

    QVariant attribute(const std::string& name) const
    {
        auto it = _attributes.find( name );

        if( it ==  _attributes.end() ) {
          return {};
        }
        else {
            return it->second;
        }
    }

    const Point& front() const
    {
        return _points.front();
    }

    const Point& back() const
    {
        return _points.back();
    }

    ConstIterator begin() const
    {
        return _points.begin();
    }

    ConstIterator end() const
    {
        return _points.end();
    }

    Iterator begin()
    {
        return _points.begin();
    }

    Iterator end()
    {
        return _points.end();
    }

    // template specialization for types that support compare operator
    virtual RangeOpt rangeX() const
    {
        if constexpr ( std::is_arithmetic_v<TypeX> )
        {
            if( _points.empty() )
            {
                return std::nullopt;
            }
            if( _range_x_dirty )
            {
              _range_x.min = front().x;
              _range_x.max = _range_x.min;
              for(const auto& p: _points)
              {
                _range_x.min = std::min(_range_x.min, p.x);
                _range_x.max = std::max(_range_x.max, p.x);
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
        if constexpr ( std::is_arithmetic_v<Value> )
        {
            if( _points.empty() )
            {
                return std::nullopt;
            }
            if( _range_y_dirty )
            {
              _range_y.min = front().y;
              _range_y.max = _range_y.min;
              for(const auto& p: _points)
              {
                _range_y.min = std::min(_range_y.min, p.y);
                _range_y.max = std::max(_range_y.max, p.y);
              }
              _range_y_dirty = false;
            }
            return _range_y;
        }
        return std::nullopt;
    }

    void pushBack(const Point &p)
    {
        auto temp = p;
        pushBack(std::move(temp));
    }

    virtual void pushBack(Point&& p)
    {
        if constexpr ( std::is_arithmetic_v<TypeX> )
        {
            if (std::isinf(p.x) || std::isnan(p.x))
            {
                return;  // skip
            }
            pushUpdateRangeX(p);
        }
        if constexpr ( std::is_arithmetic_v<Value> )
        {
            if (std::isinf(p.y) || std::isnan(p.y))
            {
                return;  // skip
            }
            pushUpdateRangeY(p);
        }

        _points.emplace_back(p);
    }

    virtual void insert(Iterator it, Point&& p)
    {
        if constexpr ( std::is_arithmetic_v<TypeX> )
        {
            if (std::isinf(p.x) || std::isnan(p.x))
            {
                return;  // skip
            }
            pushUpdateRangeX(p);
        }
        if constexpr ( std::is_arithmetic_v<Value> )
        {
            if (std::isinf(p.y) || std::isnan(p.y))
            {
                return;  // skip
            }
            pushUpdateRangeY(p);
        }

        _points.insert(it, p);
    }

    virtual void popFront()
    {
        const auto& p = _points.front();

        if constexpr ( std::is_arithmetic_v<TypeX> )
        {
            if( !_range_x_dirty && (p.x == _range_x.max || p.x == _range_x.min) )
            {
                _range_x_dirty = true;
            }
        }

        if constexpr ( std::is_arithmetic_v<Value> )
        {
            if( !_range_y_dirty && (p.y == _range_y.max || p.y == _range_y.min) )
            {
                _range_y_dirty = true;
            }
        }
        _points.pop_front();
    }

protected:
    std::string _name;
    Attributes _attributes;
    std::deque<Point> _points;

    mutable Range _range_x;
    mutable Range _range_y;
    mutable bool _range_x_dirty;
    mutable bool _range_y_dirty;
    mutable  std::shared_ptr<PlotGroup> _group;

    // template specialization for types that support compare operator
    virtual void pushUpdateRangeX(const Point& p)
    {
        if constexpr ( std::is_arithmetic_v<TypeX> )
        {
            if( _points.empty() )
            {
                _range_x_dirty = false;
                _range_x.min = p.x;
                _range_x.max = p.x;
            }
            if( !_range_x_dirty )
            {
                if( p.x > _range_x.max ){
                    _range_x.max = p.x;
                }
                else if( p.x < _range_x.min ){
                    _range_x.min = p.x;
                }
                else{
                    _range_x_dirty = true;
                }
            }
        }
    }

    // template specialization for types that support compare operator
    virtual void pushUpdateRangeY(const Point& p)
    {
        if constexpr ( std::is_arithmetic_v<Value> )
        {
            if( !_range_y_dirty )
            {
                if( p.y > _range_y.max ){
                    _range_y.max = p.y;
                }
                else if( p.y < _range_y.min ){
                    _range_y.min = p.y;
                }
                else{
                    _range_y_dirty = true;
                }
            }
        }
    }
};


} // end namespace

#endif
