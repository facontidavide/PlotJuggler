#ifndef PJ_PLOTDATA_H
#define PJ_PLOTDATA_H

#include "plotdatabase.h"
#include "timeseries.h"
#include "stringseries.h"

namespace PJ
{
using PlotDataXY = PlotDataBase<double, double>;
using PlotData = TimeseriesBase<double>;
using PlotDataAny = TimeseriesBase<std::any>;

/**
 * @brief The PlotDataMapRef is the main data structure used to store all the
 * timeseries in a single place.
 */

// obsolate. For back compatibility only
//using PlotDataMap = std::unordered_map<std::string, PlotData>;

using TimeseriesMap = std::unordered_map<std::string, PlotData>;
using ScatterXYMap = std::unordered_map<std::string, PlotDataXY>;
using AnySeriesMap = std::unordered_map<std::string, PlotDataAny>;
using StringSeriesMap = std::unordered_map<std::string, StringSeries>;

struct PlotDataMapRef
{
  ScatterXYMap scatter_xy;

  /// Numerical timeseries
  TimeseriesMap numeric;

  /// Timeseries that can contain any data structure.
  /// PlotJuggler can not display it natively, only plugins can manipulate them.
  AnySeriesMap user_defined;

  /// Series of strings
  StringSeriesMap strings;

  /**
   * @brief Each series can have (optionally) a group.
   * Groups can have their own properties.
   */
  std::unordered_map<std::string, PlotGroup::Ptr> groups;

  ScatterXYMap::iterator addScatterXY(const std::string& name, PlotGroup::Ptr group = {});

  TimeseriesMap::iterator addNumeric(const std::string& name, PlotGroup::Ptr group = {});

  AnySeriesMap::iterator addUserDefined(const std::string& name,
                                        PlotGroup::Ptr group = {});

  StringSeriesMap::iterator addStringSeries(const std::string& name,
                                            PlotGroup::Ptr group = {});


  PlotDataXY& getOrCreateScatterXY(const std::string& name, PlotGroup::Ptr group = {});

  PlotData& getOrCreateNumeric(const std::string& name, PlotGroup::Ptr group = {});

  StringSeries& getOrCreateStringSeries(const std::string& name,
                                        PlotGroup::Ptr group = {});

  PlotDataAny& getOrCreateUserDefined(const std::string& name, PlotGroup::Ptr group = {});

  PlotGroup::Ptr getOrCreateGroup(const std::string& name);

  std::unordered_set<std::string> getAllNames() const;

  void clear();

  void setMaximumRangeX(double range);

  bool erase(const std::string& name);
};

template <typename Value>
inline void AddPrefixToPlotData(const std::string& prefix,
                                std::unordered_map<std::string, Value>& data)
{
  if (prefix.empty())
  {
    return;
  }

  std::vector<std::string> temp_key;
  temp_key.reserve(data.size());
  std::vector<Value> temp_value;
  temp_value.reserve(data.size());

  for (auto& it : data)
  {
    std::string key;
    key.reserve(prefix.size() + 2 + it.first.size());
    key = (it.first.front() == '/') ? (prefix + it.first) : (prefix + "/" + it.first);

    temp_key.emplace_back(key);
    temp_value.emplace_back(std::move(it.second));
  }

  data.clear();

  for (size_t i = 0; i < temp_key.size(); i++)
  {
    const std::string& key = temp_key[i];

    auto it = data.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                           std::forward_as_tuple(key, PlotGroup::Ptr()))
                  .first;

    it->second = std::move(temp_value[i]);
  }
}

}  // namespace PJ

#endif  // PJ_PLOTDATA_H
