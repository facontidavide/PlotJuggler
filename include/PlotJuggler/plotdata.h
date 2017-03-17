#ifndef PLOTDATA_RAW_H
#define PLOTDATA_RAW_H

#include <vector>
#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <PlotJuggler/optional.hpp>
#include <PlotJuggler/any.hpp>
#include <QDebug>
#include <QColor>
#include <type_traits>

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

  PlotDataGeneric();

  PlotDataGeneric(const char* name);

  virtual ~PlotDataGeneric() {}

  void setName(const std::string& name) { _name = name; }
  std::string name() const { return _name; }

  virtual size_t size() const;

  int getIndexFromX(Time x) const;

  nonstd::optional<Value> getYfromX(Time x ) const;

  Point at(size_t index) const;

  void clear();

  void pushBack(Point p);

  void pushBackAsynchronously(Point p);

  bool flushAsyncBuffer();

  QColor getColorHint() const;

  void setColorHint(QColor color);

  void setMaximumRangeX(Time max_range);

  static std::mutex& asyncPushMutex() { return _mutex; }

protected:

  std::string _name;
  std::deque<Time>  _x_points;
  std::deque<Value> _y_points;

  std::deque<Point> _pushed_points;

  QColor _color_hint;

private:

  Time _max_range_X;
  static std::mutex _mutex;
};


typedef PlotDataGeneric<double,double>  PlotData;
typedef PlotDataGeneric<double, nonstd::any> PlotDataAny;


typedef std::shared_ptr<PlotData>     PlotDataPtr;
typedef std::shared_ptr<PlotDataAny>  PlotDataAnyPtr;

typedef struct{
  std::map<std::string, PlotDataPtr>     numeric;
  std::map<std::string, PlotDataAnyPtr>  user_defined;
} PlotDataMap;


//-----------------------------------

template < typename Time, typename Value>
std::mutex PlotDataGeneric<Time, Value>::_mutex;


template < typename Time, typename Value>
inline PlotDataGeneric<Time, Value>::PlotDataGeneric():
  _max_range_X( std::numeric_limits<Time>::max() )
  , _color_hint(Qt::black)
{
    static_assert( std::is_arithmetic<Time>::value ,"Only numbers can be used as time");
}

template<typename Time, typename Value>
inline PlotDataGeneric<Time, Value>::PlotDataGeneric(const char *name):
    _max_range_X( std::numeric_limits<Time>::max() )
    , _color_hint(Qt::black)
    , _name(name)
{
    static_assert( std::is_arithmetic<Time>::value ,"Only numbers can be used as time");
}

template < typename Time, typename Value>
inline void PlotDataGeneric<Time, Value>::pushBack(Point point)
{
  _x_points.push_back( point.x );
  _y_points.push_back( point.y );
}

template < typename Time, typename Value>
inline void PlotDataGeneric<Time, Value>::pushBackAsynchronously(Point point)
{
  _pushed_points.push_back( point );
  while(_pushed_points.size() > ASYNC_BUFFER_CAPACITY) _pushed_points.pop_front();
}

template < typename Time, typename Value>
inline bool PlotDataGeneric<Time, Value>::flushAsyncBuffer()
{
 // std::lock_guard<std::mutex> lock(_mutex);
  if( _pushed_points.empty() ) return false;

  while( !_pushed_points.empty() )
  {
      const Point& point = _pushed_points.front();
      _x_points.push_back( point.x );
      _y_points.push_back( point.y );
      _pushed_points.pop_front();
  }

  while( _x_points.size()>2 &&
         _x_points.back() - _x_points.front() > _max_range_X)
  {
      _x_points.pop_front();
      _y_points.pop_front();
  }
  return true;
}


template < typename Time, typename Value>
inline int PlotDataGeneric<Time, Value>::getIndexFromX(Time x ) const
{
  if( _x_points.size() == 0 ){
    return -1;
  }
  auto lower = std::lower_bound(_x_points.begin(), _x_points.end(), x );
  auto index = std::distance( _x_points.begin(), lower);

  if( index >= _x_points.size() || index <0 )
  {
    return -1;
  }
  return index;
}


template < typename Time, typename Value>
inline nonstd::optional<Value> PlotDataGeneric<Time, Value>::getYfromX(Time x) const
{
  auto lower = std::lower_bound(_x_points.begin(), _x_points.end(), x );
  auto index = std::distance( _x_points.begin(), lower);

  if( index >= _x_points.size() || index < 0 )
  {
    return nonstd::optional<Value>();
  }
  return _y_points.at(index);
}

template < typename Time, typename Value>
inline typename PlotDataGeneric<Time, Value>::Point
PlotDataGeneric<Time, Value>::at(size_t index) const
{
  try{
    return { _x_points[index],  _y_points[index]  };
  }
  catch(...)
  {
    return { _x_points.back(),  _y_points.back() };
    }
}

template<typename Time, typename Value>
void PlotDataGeneric<Time, Value>::clear()
{
    _x_points.clear();
    _y_points.clear();
}


template < typename Time, typename Value>
inline size_t PlotDataGeneric<Time, Value>::size() const
{
  return _x_points.size();
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
}

#endif // PLOTDATA_H
