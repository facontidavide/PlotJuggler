#include "lua_custom_function.h"
#include <QTextStream>

LuaCustomFunction::LuaCustomFunction(SnippetData snippet) : CustomFunction(snippet)
{
  initEngine();
  {
    QTextStream in(&snippet.global_vars);
    while (!in.atEnd())
    {
      in.readLine();
      global_lines_++;
    }
  }
  {
    QTextStream in(&snippet.function);
    while (!in.atEnd())
    {
      in.readLine();
      function_lines_++;
    }
  }
}

void LuaCustomFunction::initEngine()
{
  std::unique_lock<std::mutex> lk(mutex_);

  _lua_function = {};
  _lua_engine = {};
  _lua_engine.open_libraries();
  auto result = _lua_engine.safe_script(_snippet.global_vars.toStdString());
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(getError(err));
  }

  auto calcMethodStr = QString("function calc(time, value");
  for (int i = 0; i < _snippet.additional_sources.size(); i++)
  {
    if (_snippet.additional_sources[i] != _snippet.linked_source)
    {
      calcMethodStr += QString(", v%1").arg(i + 1);
    }
  }
  calcMethodStr += QString(")\n%1\nend").arg(snippet().function);

  result = _lua_engine.safe_script(calcMethodStr.toStdString());
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(getError(err));
  }
  _lua_function = _lua_engine["calc"];
}

void LuaCustomFunction::parseLuaResult(sol::safe_function_result& result, double time,
                                       std::vector<PlotData::Point>& points)
{
  if (!result.valid())
  {
    sol::error err = result;
    throw std::runtime_error(getError(err));
  }

  if (result.return_count() == 2)
  {
    PlotData::Point p;
    p.x = result.get<double>(0);
    p.y = result.get<double>(1);
    points.push_back(p);
  }
  else if (result.return_count() == 1 && result.get_type(0) == sol::type::number)
  {
    PlotData::Point p;
    p.x = time;
    p.y = result.get<double>(0);
    points.push_back(p);
  }
  else if (result.return_count() == 1 && result.get_type(0) == sol::type::table)
  {
    std::vector<std::array<double, 2>> multi_samples =
        result.get<std::vector<std::array<double, 2>>>(0);
    for (const auto& sample : multi_samples)
    {
      PlotData::Point p;
      p.x = sample[0];
      p.y = sample[1];
      points.push_back(p);
    }
  }
  else
  {
    throw std::runtime_error("Wrong return object: expecting either a single value, "
                             "two values (time, value) "
                             "or an array of two-sized arrays (time, value)");
  }
}

void LuaCustomFunction::calculatePoints(const MixedSource& main_src,
                                        const std::vector<MixedSource>& additional_src,
                                        size_t point_index, std::vector<PlotData::Point>& points)
{
  std::unique_lock<std::mutex> lk(mutex_);

  std::vector<sol::object> args;
  args.reserve(2 + additional_src.size());

  double time;
  if (main_src.is_string)
  {
    time = main_src.str->at(point_index).x;
    std::string val(main_src.str->getString(main_src.str->at(point_index).y));
    args.push_back(sol::make_object(_lua_engine, time));
    args.push_back(sol::make_object(_lua_engine, val));
  }
  else
  {
    const auto& p = main_src.numeric->at(point_index);
    time = p.x;
    args.push_back(sol::make_object(_lua_engine, time));
    args.push_back(sol::make_object(_lua_engine, p.y));
  }

  for (const auto& src : additional_src)
  {
    if (src.is_string)
    {
      int idx = src.str->getIndexFromX(time);
      std::string val =
          (idx != -1) ? std::string(src.str->getString(src.str->at(idx).y)) : std::string();
      args.push_back(sol::make_object(_lua_engine, val));
    }
    else
    {
      int idx = src.numeric->getIndexFromX(time);
      double val = (idx != -1) ? src.numeric->at(idx).y : std::numeric_limits<double>::quiet_NaN();
      args.push_back(sol::make_object(_lua_engine, val));
    }
  }

  sol::safe_function_result result = _lua_function(sol::as_args(args));
  parseLuaResult(result, time, points);
}

bool LuaCustomFunction::xmlLoadState(const QDomElement& parent_element)
{
  bool ret = CustomFunction::xmlLoadState(parent_element);
  initEngine();
  return ret;
}

std::string LuaCustomFunction::getError(sol::error err)
{
  std::string out;
  auto parts = QString(err.what()).split(":");

  if (parts.size() < 3)
  {
    return err.what();
  }

  bool is_function = parts[0].contains("[string \"function calc(time,");
  out = is_function ? "[Function]: line " : "[Global]: line ";

  int line_num = parts[1].toInt();
  if (is_function)
  {
    line_num -= 1;
  }
  out += std::to_string(line_num) + ": ";
  out += parts[2].toStdString();
  return out;
}
