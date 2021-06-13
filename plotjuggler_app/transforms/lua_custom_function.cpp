#include "lua_custom_function.h"

LuaCustomFunction::LuaCustomFunction(const SnippetData& snippet)
  : CustomFunction(snippet)
{
  initEngine();
}

void LuaCustomFunction::initEngine()
{
  std::unique_lock<std::mutex> lk(mutex_);

  _lua_engine = std::unique_ptr<sol::state>(new sol::state());
  _lua_engine->open_libraries();
  _lua_engine->script(_snippet.globalVars.toStdString());

  auto calcMethodStr = QString("function calc(time, value");
  for(int index = 1; index <= _snippet.additionalSources.size(); index++ )
  {
    calcMethodStr += QString(", v%1").arg(index);
  }
  calcMethodStr += QString(")\n%1\nend").arg(snippet().function);

  _lua_engine->safe_script(calcMethodStr.toStdString());

  _lua_function = (*_lua_engine)["calc"];
}

void LuaCustomFunction::calculatePoints(const PlotData& src_data,
                                        const std::vector<const PlotData*>& channels_data,
                                        size_t point_index,
                                        std::vector<PlotData::Point> &points)
{
  std::unique_lock<std::mutex> lk(mutex_);

  _chan_values.resize(channels_data.size());

  const PlotData::Point& old_point = src_data.at(point_index);

  for (size_t chan_index = 0; chan_index < channels_data.size(); chan_index++)
  {
    double value;
    const auto& chan_data = channels_data[chan_index];
    int index = chan_data->getIndexFromX(old_point.x);
    if (index != -1)
    {
      value = chan_data->at(index).y;
    }
    else
    {
      value = std::numeric_limits<double>::quiet_NaN();
    }
    _chan_values[chan_index] = value;
  }

  sol::safe_function_result result;
  const auto& v = _chan_values;
  // ugly code, sorry
  switch( _snippet.additionalSources.size() )
  {
  case 0: result = _lua_function(old_point.x, old_point.y);
    break;
  case 1: result = _lua_function(old_point.x, old_point.y,
                                 v[0]);
    break;
  case 2: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1]);
    break;
  case 3: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2]);
    break;
  case 4: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2], v[3]);
    break;
  case 5: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2], v[3], v[4]);
    break;
  case 6: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2], v[3], v[4], v[5]);
    break;
  case 7: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
    break;
  case 8: result = _lua_function(old_point.x, old_point.y,
                                 v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
    break;
  default:
    throw std::runtime_error("Lua Engine : maximum number of additional source is 8");
  }

  if( !result.valid() )
  {
    throw std::runtime_error("Lua Engine : invalid function (missing variable?)");
  }

  if (result.return_count() == 2)
  {
    PlotData::Point new_point;
    new_point.x = result.get<double>(0);
    new_point.y = result.get<double>(1);
    points.push_back(new_point);
  }
  else if (result.return_count() == 1 && result.get_type(0) == sol::type::number)
  {
    PlotData::Point new_point;
    new_point.x = old_point.x;
    new_point.y = result.get<double>(0);
    points.push_back(new_point);
  }
  else if (result.return_count() == 1 && result.get_type(0) == sol::type::table)
  {
    static std::vector<std::array<double, 2>> multi_samples;
    multi_samples.clear();

    multi_samples = result.get<std::vector<std::array<double, 2>>>(0);

    for (std::array<double, 2> sample : multi_samples)
    {
      PlotData::Point point;
      point.x = sample[0];
      point.y = sample[1];
      points.push_back(point);
    }
  }
  else
  {
    throw std::runtime_error("Lua Engine : return either a single value, two values (time, value) "
                             "or an array of two-sized arrays (time, value)");
  }
}
