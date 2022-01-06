#include "PlotJuggler/reactive_function.h"
#include <sol/sol.hpp>

namespace PJ
{

ReactiveLuaFunction::ReactiveLuaFunction(PlotDataMapRef *data_map,
                                         QString lua_global,
                                         QString lua_function):
  _global_code(lua_global.toStdString()), _function_code(lua_function.toStdString())
{
  _data = data_map;
  _lua_function = {};
  _lua_engine = {};

  _lua_engine.open_libraries(sol::lib::base);
  _lua_engine.open_libraries(sol::lib::string);

  prepareLua();

  auto calcFunction = QString("function calc(tracker_time)\n%1\nend").arg(lua_function);

  sol::protected_function_result result = _lua_engine.safe_script(_global_code);
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(std::string("Error in Global part:\n") + err.what());
  }

  result = _lua_engine.script(calcFunction.toStdString());
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(std::string("Error in Function part:\n") + err.what());
  }
  _lua_function = _lua_engine["calc"];
}

void ReactiveLuaFunction::reset()
{
}

void ReactiveLuaFunction::setTimeTracker(double time_tracker_value)
{
  _tracker_value = time_tracker_value;
}

void ReactiveLuaFunction::calculate()
{
  _lua_function(_tracker_value);
}

void ReactiveLuaFunction::prepareLua()
{
  _timeseries_ref = _lua_engine.new_usertype<TimeseriesRef>("TimeseriesView");

  _timeseries_ref["find"] = [&](sol::object name)
  {
    if (name.is<std::string>() == false)
    {
      return sol::make_object(_lua_engine, sol::lua_nil);
    }

    auto it = plotData()->numeric.find(name.as<std::string>());
    if( it == plotData()->numeric.end() )
    {
      return sol::make_object(_lua_engine, sol::lua_nil);
    }
    return sol::object(_lua_engine, sol::in_place, TimeseriesRef( &(it->second)) );
  };
  _timeseries_ref["size"] = &TimeseriesRef::size;
  _timeseries_ref["at"] = &TimeseriesRef::at;
  _timeseries_ref["atTime"] = &TimeseriesRef::atTime;

  //---------------------------------------
  _created_timeseries = _lua_engine.new_usertype<CreatedSeries>("MutableTimeseries");

  _created_timeseries["new"] = [&](sol::object name)
  {
    if (name.is<std::string>() == false)
    {
      return sol::make_object(_lua_engine, sol::lua_nil);
    }
    auto str_name = name.as<std::string>();
    auto series = CreatedSeries(plotData(), str_name, true);
    _created_cuves.push_back( str_name );
    return sol::object(_lua_engine, sol::in_place, series);
  };

  _created_timeseries["at"] = &CreatedSeries::at;
  _created_timeseries["size"] = &CreatedSeries::size;
  _created_timeseries["clear"] = &CreatedSeries::clear;
  _created_timeseries["push_back"] = &CreatedSeries::push_back;

  //---------------------------------------
 /* sol::usertype<CreatedSeries> created_scatter =
      lua.new_usertype<CreatedSeries>("MutableScatterXY");

  created_scatter["new"] = [&](sol::object name)
  {
    if (name.is<std::string>() == false)
    {
      return sol::make_object(lua, sol::lua_nil);
    }
    auto str_name = name.as<std::string>();
    auto series = CreatedSeries(plotData(), str_name, false);
    _created_cuves.push_back( str_name );
    return sol::object(lua, sol::in_place, series);
  };

  created_scatter["at"] = &CreatedSeries::at;
  created_scatter["size"] = &CreatedSeries::size;
  created_scatter["clear"] = &CreatedSeries::clear;
  created_scatter["push_back"] = &CreatedSeries::push_back;*/
}

TimeseriesRef::TimeseriesRef(PlotData *data): _plot_data(data)
{}

std::pair<double, double> TimeseriesRef::at(unsigned i) const
{
  const auto& p = _plot_data->at(i);
  return {p.x, p.y};
}

double TimeseriesRef::atTime(double t) const
{
  int i = _plot_data->getIndexFromX(t);
  return _plot_data->at(i).y;
}

unsigned TimeseriesRef::size() const
{
  return _plot_data->size();
}

CreatedSeries::CreatedSeries(PlotDataMapRef *data_map, const std::string &name, bool timeseries)
{
  if( timeseries )
  {
    _plot_data = &(data_map->getOrCreateNumeric(name));
  }
  else
  {
    _plot_data = &(data_map->getOrCreateScatterXY(name));
  }
}

std::pair<double, double> CreatedSeries::at(unsigned i) const
{
  const auto& p = _plot_data->at(i);
  return {p.x, p.y};
}

void CreatedSeries::clear()
{
  _plot_data->clear();
}

void CreatedSeries::push_back(double x, double y)
{
  _plot_data->pushBack( {x,y} );
}

unsigned CreatedSeries::size() const
{
  return _plot_data->size();
}

}
