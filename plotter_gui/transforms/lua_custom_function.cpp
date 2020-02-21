#include "lua_custom_function.h"


LuaCustomFunction::LuaCustomFunction(const std::string &linkedPlot,
                                     const SnippetData &snippet):
CustomFunction(linkedPlot,snippet)
{
  createReplacedFunction(1);
  initEngine();
}

void LuaCustomFunction::initEngine()
{
  _lua_engine = std::unique_ptr<sol::state>( new sol::state() );
  _lua_engine->open_libraries();
  _lua_engine->script(_global_vars.toStdString());

  QString calcMethodStr = QString("function calc(time, value, CHANNEL_VALUES) %1 end").arg(_function_replaced);
  _lua_engine->script(calcMethodStr.toStdString());

  _lua_function = (*_lua_engine)["calc"];
}


PlotData::Point LuaCustomFunction::calculatePoint(
  const PlotData& src_data,
  const std::vector<const PlotData*>& channels_data,
  size_t point_index)
{
  _chan_values.resize(channels_data.size());

  const PlotData::Point &old_point = src_data.at(point_index);

  for(int chan_index = 0; chan_index < channels_data.size(); chan_index++)
  {
    double value;
    const auto& chan_data = channels_data[chan_index];
    int index = chan_data->getIndexFromX(old_point.x);
    if(index != -1){
      value = chan_data->at(index).y;
    }
    else{
      value = std::numeric_limits<double>::quiet_NaN();
    }
    _chan_values[chan_index] = value;
  }

  PlotData::Point new_point;
  new_point.x = old_point.x;

  sol::function_result result = _lua_function( old_point.x,
                                              old_point.y,
                                              _chan_values );

  if( result.return_count() == 2 )
  {
    new_point.x = result.get<double>(0);
    new_point.y = result.get<double>(1);
  }
  else if( result.return_count() == 1 )
  {
    new_point.y = result.get<double>(0);
  }
  else {
    throw std::runtime_error(
      "Lua Engine : if you return an array, the size must be "
      "2 (time/value pair) or 1 (value only)");
  }
  return new_point;
}
