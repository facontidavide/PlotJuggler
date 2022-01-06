#ifndef REACTIVE_FUNCTION_H
#define REACTIVE_FUNCTION_H

#include "PlotJuggler/transform_function.h"
#include <sol/sol.hpp>

class TimeseriesRef;
class CreatedSeries;

namespace PJ {

struct TimeseriesRef
{
  TimeseriesRef(PlotData* data);

  std::pair<double, double> at(unsigned i) const;

  double atTime(double t) const;

  unsigned size() const;

  PJ::PlotData* _plot_data = nullptr;
};

//-----------------------

struct CreatedSeries
{
  CreatedSeries(PlotDataMapRef* data_map, const std::string& name, bool timeseries);

  std::pair<double, double> at(unsigned i) const;

  void clear();

  void push_back(double x, double y);

  unsigned size() const;

  PJ::PlotDataXY* _plot_data = nullptr;
};

//-----------------------

class ReactiveLuaFunction: public PJ::TransformFunction
{
public:
  ReactiveLuaFunction(PlotDataMapRef* data_map,
                      QString lua_global,
                      QString lua_function);

  const char* name() const override
  {
    return "ReactiveLuaFunction";
  }

  int numInputs() const override
  {
    return -1;
  }

  int numOutputs() const override
  {
    return -1;
  }

  void reset() override;

  void setTimeTracker(double time_tracker_value);

  void calculate() override;

  const std::vector<std::string>& createdCuves() const
  {
    return _created_cuves;
  }

protected:

  void prepareLua();

  double _tracker_value = 0;
  std::string _global_code;
  std::string _function_code;

  std::vector<std::string> _created_cuves;

  sol::state _lua_engine;
  sol::protected_function _lua_function;

  sol::usertype<TimeseriesRef> _timeseries_ref;
  sol::usertype<CreatedSeries> _created_timeseries;
};

}

#endif // REACTIVE_FUNCTION_H
