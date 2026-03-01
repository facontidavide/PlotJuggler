/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_STRINGSERIES_H
#define PJ_STRINGSERIES_H

#include "PlotJuggler/timeseries.h"
#include "PlotJuggler/string_ref_sso.h"
#include "PlotJuggler/string_dict_index.h"
#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace PJ
{
class StringSeries : public TimeseriesBase<StringDictIndex>
{
public:
  using TimeseriesBase<StringDictIndex>::_points;

  StringSeries(const std::string& name, PlotGroup::Ptr group)
    : TimeseriesBase<StringDictIndex>(name, group)
  {
  }

  StringSeries(const StringSeries& other) = delete;
  StringSeries(StringSeries&& other) = default;

  StringSeries& operator=(const StringSeries& other) = delete;
  StringSeries& operator=(StringSeries&& other) = default;

  virtual void clear() override
  {
    _index_to_string.clear();
    _string_to_index.clear();
    TimeseriesBase<StringDictIndex>::clear();
  }

  void pushBack(const Point& p) override
  {
    // Point.y is StringDictIndex — forward to base
    TimeseriesBase<StringDictIndex>::pushBack(p);
  }

  void pushBack(Point&& p) override
  {
    TimeseriesBase<StringDictIndex>::pushBack(std::move(p));
  }

  // Backward-compatible overload: accepts {timestamp, StringRef}
  void pushBack(std::pair<double, StringRef> p)
  {
    const auto& str = p.second;
    if (str.data() == nullptr || str.size() == 0)
    {
      return;
    }
    StringDictIndex idx = internString(std::string_view(str.data(), str.size()));
    TimeseriesBase<StringDictIndex>::pushBack({ p.first, idx });
  }

  std::string_view getString(StringDictIndex idx) const
  {
    if (!idx.isValid() || idx.index >= _index_to_string.size())
    {
      return {};
    }
    return _index_to_string[idx.index];
  }

  std::optional<std::string_view> getStringFromX(double x) const
  {
    int index = getIndexFromX(x);
    if (index < 0)
    {
      return std::nullopt;
    }
    return getString(_points[index].y);
  }

  void clonePoints(StringSeries&& other)
  {
    _index_to_string = std::move(other._index_to_string);
    _string_to_index = std::move(other._string_to_index);
    PlotDataBase<double, StringDictIndex>::clonePoints(std::move(other));
  }

  void clonePoints(const StringSeries& other)
  {
    _index_to_string = other._index_to_string;
    _string_to_index = other._string_to_index;
    PlotDataBase<double, StringDictIndex>::clonePoints(other);
  }

private:
  StringDictIndex internString(std::string_view str)
  {
    _tmp_str.assign(str.data(), str.size());
    auto it = _string_to_index.find(_tmp_str);
    if (it != _string_to_index.end())
    {
      return StringDictIndex(it->second);
    }
    uint32_t new_index = static_cast<uint32_t>(_index_to_string.size());
    _index_to_string.push_back(_tmp_str);
    _string_to_index.emplace(_tmp_str, new_index);
    return StringDictIndex(new_index);
  }

  std::string _tmp_str;
  std::vector<std::string> _index_to_string;
  std::unordered_map<std::string, uint32_t> _string_to_index;
};

}  // namespace PJ

#endif
