#include "PlotJuggler/reactive_function.h"
#include <sol/sol.hpp>
#include <PlotJuggler/fmt/format.h>

namespace PJ
{

void ReactiveLuaFunction::init()
{
  _lua_function = {};
  _lua_engine = {};

  _lua_engine.open_libraries(sol::lib::base);
  _lua_engine.open_libraries(sol::lib::string);
  _lua_engine.open_libraries(sol::lib::math);

  _lua_engine.script(_library_code);

  prepareLua();

  sol::protected_function_result result = _lua_engine.safe_script(_library_code);
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(std::string("Error in Library part:\n") + err.what());
  }

  result = _lua_engine.safe_script(_global_code);
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(std::string("Error in Global part:\n") + err.what());
  }

  auto calcFunction = fmt::format("function calc(tracker_time)\n{}\nend", _function_code);
  result = _lua_engine.script(calcFunction);
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(std::string("Error in Function part:\n") + err.what());
  }
  _lua_function = _lua_engine["calc"];
}

ReactiveLuaFunction::ReactiveLuaFunction(PlotDataMapRef *data_map,
                                         QString lua_global,
                                         QString lua_function,
                                         QString lua_library):
  _global_code(lua_global.toStdString()),
  _function_code(lua_function.toStdString()),
  _library_code(lua_library.toStdString())
{
  _data = data_map;
  init();
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

bool ReactiveLuaFunction::xmlSaveState(QDomDocument &, QDomElement &) const
{
  return false;
}

bool ReactiveLuaFunction::xmlLoadState(const QDomElement &)
{
  return false;
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
  _created_timeseries = _lua_engine.new_usertype<CreatedSeriesTime>("MutableTimeseries");

  _created_timeseries["new"] = [&](sol::object name)
  {
    if (name.is<std::string>() == false)
    {
      return sol::make_object(_lua_engine, sol::lua_nil);
    }
    auto str_name = name.as<std::string>();
    auto series = CreatedSeriesTime(plotData(), str_name);
    series.clear();
    _created_curves.push_back( str_name );
    return sol::object(_lua_engine, sol::in_place, series);
  };

  _created_timeseries["at"] = &CreatedSeriesTime::at;
  _created_timeseries["size"] = &CreatedSeriesTime::size;
  _created_timeseries["clear"] = &CreatedSeriesTime::clear;
  _created_timeseries["push_back"] = &CreatedSeriesTime::push_back;

  //---------------------------------------
  _created_scatter = _lua_engine.new_usertype<CreatedSeriesXY>("MutableScatterXY");

  _created_scatter["new"] = [&](sol::object name)
  {
    if (name.is<std::string>() == false)
    {
      return sol::make_object(_lua_engine, sol::lua_nil);
    }
    auto str_name = name.as<std::string>();
    auto series = CreatedSeriesXY(plotData(), str_name);
    series.clear();
    _created_curves.push_back( str_name );
    return sol::object(_lua_engine, sol::in_place, series);
  };

  _created_scatter["at"] = &CreatedSeriesXY::at;
  _created_scatter["size"] = &CreatedSeriesXY::size;
  _created_scatter["clear"] = &CreatedSeriesXY::clear;
  _created_scatter["push_back"] = &CreatedSeriesXY::push_back;

  //---------------------------------------
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

CreatedSeriesBase::CreatedSeriesBase(PlotDataMapRef *data_map, const std::string &name, bool timeseries)
{
  if( timeseries )
  {
    std::cout << "[DEBUG] Create timeserie, bool timeseries=" << timeseries << "\n";
    _plot_data = &(data_map->getOrCreateNumeric(name));
  }
  else
  {
    std::cout << "[DEBUG] Create scatterplot, bool timeseries=" << timeseries << "\n";
    _plot_data = &(data_map->getOrCreateScatterXY(name));
  }
}

std::pair<double, double> CreatedSeriesBase::at(unsigned i) const
{
  const auto& p = _plot_data->at(i);
  return {p.x, p.y};
}

void CreatedSeriesBase::clear()
{
  _plot_data->clear();
}

void CreatedSeriesBase::push_back(double x, double y)
{
  _plot_data->pushBack( {x,y} );
}

unsigned CreatedSeriesBase::size() const
{
  return _plot_data->size();
}


CreatedSeriesTime::CreatedSeriesTime(PlotDataMapRef *data_map, const std::string &name) : 
  CreatedSeriesBase(data_map, name, true)
{}

CreatedSeriesXY::CreatedSeriesXY(PlotDataMapRef *data_map, const std::string &name) : 
  CreatedSeriesBase(data_map, name, false)
{}

}