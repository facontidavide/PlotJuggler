/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_STRINGSERIES_H
#define PJ_STRINGSERIES_H

#include "PlotJuggler/timeseries.h"
#include "PlotJuggler/string_ref_sso.h"
#include <algorithm>
#include <unordered_set>

namespace PJ
{
class StringSeries : public TimeseriesBase<StringRef>
{
public:
  using TimeseriesBase<StringRef>::_points;

  StringSeries(const std::string& name, PlotGroup::Ptr group)
    : TimeseriesBase<StringRef>(name, group)
  {
  }

  StringSeries(const StringSeries& other) = delete;
  StringSeries(StringSeries&& other) = default;

  StringSeries& operator=(const StringSeries& other) = delete;
  StringSeries& operator=(StringSeries&& other) = default;

  virtual void clear() override
  {
    _storage.clear();
    TimeseriesBase<StringRef>::clear();
  }

  void pushBack(const Point& p) override
  {
    auto temp = p;
    pushBack(std::move(temp));
  }

  virtual void pushBack(Point&& p) override
  {
    const auto& str = p.y;
    // do not add empty strings
    if (str.data() == nullptr || str.size() == 0)
    {
      return;
    }
    if (str.isSSO())
    {
      // the object stored the string already, just push it
      TimeseriesBase<StringRef>::pushBack(std::move(p));
    }
    else
    {
      // save a copy of the string in the flywheel structure _storage
      // create a reference to that cached value.
      _tmp_str.assign(str.data(), str.size());

      auto it = _storage.find(_tmp_str);
      if (it == _storage.end())
      {
        it = _storage.insert(_tmp_str).first;
      }
      TimeseriesBase<StringRef>::pushBack({ p.x, StringRef(*it) });
    }
  }

  virtual void mergeWith(PlotDataBase<double, StringRef>& other) override {
    StringSeries* otherStringSeries = dynamic_cast<StringSeries*>(&other);
    if (otherStringSeries) {
      for (auto& pointRef : other) {
        if (pointRef.y.isSSO()){
          continue;
        }

        _tmp_str.assign(pointRef.y.data(), pointRef.y.size());

        auto it = _storage.find(_tmp_str);
        if (it == _storage.end())
        {
          it = _storage.insert(_tmp_str).first;
        }

        pointRef.y = std::move(StringRef(*it)); // point pointers to this here storage
      }
    }
    PlotDataBase<double, StringRef>::mergeWith(other);
  }

private:
  std::string _tmp_str;
  std::unordered_set<std::string> _storage;
};

}  // namespace PJ

#endif
