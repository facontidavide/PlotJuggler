#ifndef PJ_PLOTDATA_H
#define PJ_PLOTDATA_H

#include "plotdatabase.h"
#include "timeseries.h"
#include "stringseries.h"

namespace PJ {

using PlotDataXY = PlotDataBase<double, double>;
using PlotData = TimeseriesBase<double>;
using PlotDataAny = TimeseriesBase<std::any>;

struct PlotDataMapRef
{
  std::unordered_map<std::string, PlotData> numeric;
  std::unordered_map<std::string, PlotDataAny> user_defined;
  std::unordered_map<std::string, StringSeries> strings;
  std::unordered_map<std::string, PlotGroup::Ptr> groups;

  template <typename T>
  typename std::unordered_map<std::string, T>::iterator addImpl(
      std::unordered_map<std::string, T>& series, const std::string& name, PlotGroup::Ptr group )
  {
    std::string ID;
    if( group )
    {
      ID = group->name();
      if (ID.back() != '/')
      {
        ID.push_back('/');
      }
    }
    ID += name;

    return series.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(name),
          std::forward_as_tuple(name, group)).first;
  }

  template <typename T>
  T& getOrCreateImpl(std::unordered_map<std::string, T>& series, const std::string& name, const PlotGroup::Ptr& group)
  {
    auto it = series.find( name );
    if( it == series.end() ) {
      it = addImpl(series, name, group);
    }
    return it->second;
  }

  std::unordered_map<std::string, PlotData>::iterator addNumeric(const std::string& name, PlotGroup::Ptr group = {})
  {
    return addImpl(numeric, name, group);
  }

  std::unordered_map<std::string, PlotDataAny>::iterator addUserDefined(const std::string& name, PlotGroup::Ptr group = {})
  {
    return addImpl(user_defined, name, group);
  }

  std::unordered_map<std::string, StringSeries>::iterator addStringSeries(const std::string& name, PlotGroup::Ptr group = {})
  {
    return addImpl(strings, name, group);
  }

  PlotData& getOrCreateNumberic(const std::string& name, PlotGroup::Ptr group = {})
  {
    return getOrCreateImpl( numeric, name, group );
  }

  StringSeries& getOrCreateStringSeries(const std::string& name, PlotGroup::Ptr group = {})
  {
    return getOrCreateImpl( strings, name, group );
  }

  PlotDataAny& getOrCreateUserDefined(const std::string& name, PlotGroup::Ptr group = {})
  {
    return getOrCreateImpl( user_defined, name, group );
  }

  PlotGroup::Ptr getOrCreateGroup(const std::string& name)
  {
    if( name.empty() ) {
      throw std::runtime_error( "Group name can not be empty" );
    }
    auto& group = groups[ name ];
    if( !group ) {
      group = std::make_shared<PlotGroup>(name);
    }
    return group;
  }

  void clear()
  {
    numeric.clear();
    strings.clear();
    user_defined.clear();
  }

  void setMaximumRangeX( double range )
  {
    for (auto& it : numeric)
    {
      it.second.setMaximumRangeX( range );
    }
    for (auto& it : strings)
    {
      it.second.setMaximumRangeX( range );
    }
    for (auto& it : user_defined)
    {
      it.second.setMaximumRangeX( range );
    }
  }

  bool erase(const std::string& name )
  {
    bool erased = false;
    auto num_it = numeric.find(name);
    if (num_it != numeric.end())
    {
      numeric.erase( num_it );
      erased = true;
    }

    auto str_it = strings.find(name);
    if (str_it != strings.end())
    {
      strings.erase( str_it );
      erased = true;
    }

    auto any_it = user_defined.find(name);
    if (any_it != user_defined.end())
    {
      user_defined.erase( any_it );
      erased = true;
    }
    return erased;
  }

};

template <typename Value>
inline void AddPrefixToPlotData(const std::string& prefix,
                                std::unordered_map<std::string, Value>& data)
{
  if (prefix.empty()){
    return;
  }

  std::vector<std::string> temp_key;
  temp_key.reserve( data.size() );
  std::vector<Value> temp_value;
  temp_value.reserve( data.size() );

  for (auto& it : data)
  {
    std::string key;
    key.reserve(prefix.size() + 2 + it.first.size());
    key =  (it.first.front() == '/') ? (prefix + it.first) : (prefix + "/" + it.first);

    temp_key.emplace_back( key );
    temp_value.emplace_back( std::move(it.second) );
  }

  data.clear();

  for (size_t i=0; i < temp_key.size(); i++)
  {
    const std::string& key = temp_key[i];

    auto it = data.emplace(std::piecewise_construct,
                           std::forward_as_tuple(key),
                           std::forward_as_tuple(key, PlotGroup::Ptr()) ).first;

    it->second = std::move(temp_value[i]);
  }
}

}

#endif // PJ_PLOTDATA_H
