#ifndef LUA_CUSTOM_FUNCTION_H
#define LUA_CUSTOM_FUNCTION_H

#include "custom_function.h"
#include "sol.hpp"

class LuaCustomFunction : public CustomFunction
{
public:
  LuaCustomFunction(const SnippetData& snippet);

  void initEngine() override;

  std::vector<PlotData::Point> calculatePoints(const PlotData& src_data,
                                               const std::vector<const PlotData*>& channels_data,
                                               size_t point_index) override;

  QString language() const override
  {
    return "LUA";
  }

private:
  std::unique_ptr<sol::state> _lua_engine;
  sol::protected_function _lua_function;
  std::vector<double> _chan_values;
  std::mutex mutex_;
};

#endif  // LUA_CUSTOM_FUNCTION_H
