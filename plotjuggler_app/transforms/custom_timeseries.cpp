#include "custom_timeseries.h"
#include <QSettings>
#include "lua_custom_function.h"

//CustomTimeseries::CustomTimeseries(const Timeseries* source_data, const SnippetData& snippet, PlotDataMapRef& mapped_data)
//  : TimeseriesQwt(source_data, &_cached_data), _mapped_data(mapped_data)
//{
//  if (snippet.language == "LUA")
//  {
//    _transform = std::make_unique<LuaCustomFunction>(source_data->name(), snippet);
//  }
//  else  // JS by default for back compatibility
//  {
//    _transform = std::make_unique<JsCustomFunction>(source_data->name(), snippet);
//  }

//  updateCache();
//}

//bool CustomTimeseries::updateCache()
//{
//  if (_source_data->size() == 0)
//  {
//    _cached_data.clear();
//    _bounding_box = QRectF();
//    return true;
//  }

//  _transform->calculate(_mapped_data, &_cached_data);
//  calculateBoundingBox();

//  return true;
//}
