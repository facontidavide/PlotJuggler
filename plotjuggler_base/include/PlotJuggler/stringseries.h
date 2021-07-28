#ifndef PJ_STRINGSERIES_H
#define PJ_STRINGSERIES_H

#include "PlotJuggler/timeseries.h"
#include "PlotJuggler/string_ref_sso.h"
#include <algorithm>
#include <unordered_set>

namespace PJ {

class StringSeries: public TimeseriesBase<StringRef>
{
public:
    using TimeseriesBase<StringRef>::_points;

    StringSeries(const std::string& name, PlotGroup::Ptr group):
        TimeseriesBase<StringRef>(name, group)
    { }

    StringSeries(const StringSeries& other) = delete;
    StringSeries(StringSeries&& other) = default;

    StringSeries& operator=(const StringSeries& other) = delete;
    StringSeries& operator=(StringSeries&& other) = default;

    virtual void clear() override
    {
        _storage.clear();
        TimeseriesBase<StringRef>::clear();
    }

    void pushBack(typename std::deque<Point>::iterator start, typename std::deque<Point>::iterator end) override {
        TimeseriesBase<StringRef>::pushBack(start, end);
    }

    void pushBack(const Point &p)  override
    {
        auto temp = p;
        pushBack(std::move(temp));
    }

    virtual void pushBack(Point&& p) override
    {
      const auto& str = p.y;
        // do not add empty strings
        if ( str.data() == nullptr || str.size() == 0)
        {
            return;
        }
        if( str.isSSO() )
        {
          // the object stroed the string already, just push it
          TimeseriesBase<StringRef>::pushBack( std::move(p) );
        }
        else
        {
          // save a copy of the string in the flywheel structure _storage
          // create a reference to that cached value.
          _tmp_str.assign( str.data(), str.size() );

          auto it = _storage.find( _tmp_str );
          if( it == _storage.end() ) {
            it = _storage.insert( _tmp_str ).first;
          }
          TimeseriesBase<StringRef>::pushBack( { p.x, StringRef( *it ) } );
        }
    }

private:
    std::string _tmp_str;
    std::unordered_set<std::string> _storage;
};


} // end namespace

#endif
