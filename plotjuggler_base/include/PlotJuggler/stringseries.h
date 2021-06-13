#ifndef PJ_STRINGSERIES_H
#define PJ_STRINGSERIES_H

#include "timeseries.h"
#include <algorithm>
#include <unordered_set>

namespace PJ {

class StringSeries: public TimeseriesBase<std::string_view>
{
public:
    using TimeseriesBase<std::string_view>::_points;

    StringSeries(const std::string& name, PlotGroup::Ptr group):
        TimeseriesBase<std::string_view>(name, group)
    { }

    StringSeries(const StringSeries& other) = delete;
    StringSeries(StringSeries&& other) = default;

    StringSeries& operator=(const StringSeries& other) = delete;
    StringSeries& operator=(StringSeries&& other) = default;

    virtual void clear() override
    {
        _storage.clear();
        TimeseriesBase<std::string_view>::clear();
    }

    void pushBack(const Point &p)
    {
        auto temp = p;
        pushBack(std::move(temp));
    }

    virtual void pushBack(Point&& p) override
    {
        // do not add empty strings
        if ( p.y.data() == nullptr || p.y.size() == 0)
        {
            return;
        }
        _tmp_str.assign( p.y.data(), p.y.size() );

        auto it = _storage.find( _tmp_str );
        if( it == _storage.end() ) {
            it = _storage.insert( _tmp_str ).first;
        }
        // new string_view should point at local storage
        TimeseriesBase<std::string_view>::pushBack( { p.x, std::string_view ( *it ) } );
    }

private:
    std::string _tmp_str;
    std::unordered_set<std::string> _storage;
};


} // end namespace

#endif
