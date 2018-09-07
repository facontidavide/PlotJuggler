#ifndef PLOTDATA_RAW_H
#define PLOTDATA_RAW_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include "PlotJuggler/optional.hpp"
#include "PlotJuggler/any.hpp"
#include <QDebug>
#include <QColor>
#include <type_traits>
#include <unordered_map>

template <typename Time, typename Value> class PlotDataGeneric
{
public:

  struct RangeTime{
    Time min;
    Time max;
  };

  struct RangeValue{
    Value min;
    Value max;
  };

  typedef nonstd::optional<RangeTime>  RangeTimeOpt;
  typedef nonstd::optional<RangeValue> RangeValueOpt;

  class Point{
  public:
    Time x;
    Value y;
    Point( Time _x, Value _y):
        x(_x), y(_y) {}
    Point() = default;
  };

  enum{
    MAX_CAPACITY = 1024*1024,
    ASYNC_BUFFER_CAPACITY = 1024
  };

  typedef Time    TimeType;

  typedef Value   ValueType;

  PlotDataGeneric(const std::string& name);

  PlotDataGeneric( const PlotDataGeneric<Time,Value>& other) = delete;

  void swapData( PlotDataGeneric<Time,Value>& other)
  {
      std::swap(_points, other._points);
  }

  PlotDataGeneric& operator = (const PlotDataGeneric<Time,Value>& other) = delete;

  virtual ~PlotDataGeneric() {}

  const std::string &name() const { return _name; }

  virtual size_t size() const;

  int getIndexFromX(Time x) const;

  nonstd::optional<Value> getYfromX(Time x ) const;

  const Point &at(size_t index) const;

  Point &at(size_t index);

  void clear();

  void pushBack(Point p);

  QColor getColorHint() const;

  void setColorHint(QColor color);

  void setMaximumRangeX(Time max_range);

  Time maximumRangeX() const { return _max_range_X; }

  const Point& front() const { return _points.front(); }

  const Point& back() const { return _points.back(); }

  void popFront() { _points.pop_front(); }

protected:

  std::string _name;
  std::deque<Point> _points;
  QColor _color_hint;

private:
  Time _max_range_X;
};


typedef PlotDataGeneric<double,double>  PlotData;
typedef PlotDataGeneric<double, nonstd::any> PlotDataAny;
typedef std::shared_ptr<PlotData> PlotDataPtr;


typedef struct{
  std::unordered_map<std::string, PlotData>     numeric;
  std::unordered_map<std::string, PlotDataAny>  user_defined;

  std::unordered_map<std::string, PlotData>::iterator addNumeric(const std::string& name)
  {
      return numeric.emplace( std::piecewise_construct,
                       std::forward_as_tuple(name),
                       std::forward_as_tuple(name)
                       ).first;
  }


  std::unordered_map<std::string, PlotDataAny>::iterator addUserDefined(const std::string& name)
  {
      return user_defined.emplace( std::piecewise_construct,
                                   std::forward_as_tuple(name),
                                   std::forward_as_tuple(name)
                                   ).first;
  }

} PlotDataMapRef;


//-----------------------------------


//template < typename Time, typename Value>
//inline PlotDataGeneric<Time, Value>::PlotDataGeneric():
//  _max_range_X( std::numeric_limits<Time>::max() )
//  , _color_hint(Qt::black)
//{
//    static_assert( std::is_arithmetic<Time>::value ,"Only numbers can be used as time");
//}

template<typename Time, typename Value>
inline PlotDataGeneric<Time, Value>::PlotDataGeneric(const std::string &name):
    _max_range_X( std::numeric_limits<Time>::max() )
    , _color_hint(Qt::black)
    , _name(name)
{
    static_assert( std::is_arithmetic<Time>::value ,"Only numbers can be used as time");
}

template < typename Time, typename Value>
inline void PlotDataGeneric<Time, Value>::pushBack(Point point)
{
  _points.push_back( point );

  while( _points.size()>2 &&
         (_points.back().x - _points.front().x) > _max_range_X)
  {
        _points.pop_front();
  }
}

template < typename Time, typename Value>
inline int PlotDataGeneric<Time, Value>::getIndexFromX(Time x ) const
{
  if( _points.size() == 0 ){
    return -1;
  }
  auto lower = std::lower_bound(_points.begin(), _points.end(), Point(x,0),
                                [](const Point &a, const Point &b)
                                { return a.x < b.x; } );
  auto index = std::distance( _points.begin(), lower);

  if( index >= _points.size() )
  {
    return _points.size() -1;
  }
  if( index < 0)
  {
    return 0;
  }

  if( index > 0)
  {
    if( std::abs( _points[index-1].x - x) < std::abs( _points[index].x - x) )
    {
      return index-1;
    }
    else{
      return index;
    }
  }
  return index;
}


template < typename Time, typename Value>
inline nonstd::optional<Value> PlotDataGeneric<Time, Value>::getYfromX(Time x) const
{
  int index = getIndexFromX( x );
  if( index == -1 )
  {
    return nonstd::optional<Value>();
  }
  return _points.at(index).y;
}

template < typename Time, typename Value>
inline const typename PlotDataGeneric<Time, Value>::Point&
PlotDataGeneric<Time, Value>::at(size_t index) const
{
    return _points[index];
}

template < typename Time, typename Value>
inline typename PlotDataGeneric<Time, Value>::Point&
PlotDataGeneric<Time, Value>::at(size_t index)
{
    return _points[index];
}

template<typename Time, typename Value>
void PlotDataGeneric<Time, Value>::clear()
{
    _points.clear();
}


template < typename Time, typename Value>
inline size_t PlotDataGeneric<Time, Value>::size() const
{
  return _points.size();
}

template < typename Time, typename Value>
inline QColor PlotDataGeneric<Time, Value>::getColorHint() const
{
  return _color_hint;
}

template < typename Time, typename Value>
inline void PlotDataGeneric<Time, Value>::setColorHint(QColor color)
{
  _color_hint = color;
}


template < typename Time, typename Value>
inline void PlotDataGeneric<Time, Value>::setMaximumRangeX(Time max_range)
{
  _max_range_X = max_range;
  while( _points.size()>2 &&
         _points.back().x - _points.front().x > _max_range_X)
  {
        _points.pop_front();
  }
}

#endif // PLOTDATA_H
