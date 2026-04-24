#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "query.h"
#include "types.h"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

struct ValidationResult
{
  bool valid = false;
  std::string error;
  int line = 0;
  int column = 0;
};

// Lua-based metadata query engine.
//
// Usage:
//   Engine engine;
//   engine.set(metadata);
//   auto ok = engine.eval(query);   // Query object or string_view
//
// Metadata keys are injected as Lua globals. The query is expanded
// (shorthand resolved) by the Query class before Lua sees it.
class Engine
{
public:
  Engine()
  {
    lua_.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table);
  }

  void set(const Metadata& metadata)
  {
    for (const auto& [key, value] : metadata)
    {
      if (auto num = try_number(value))
      {
        lua_[key] = *num;
      }
      else
      {
        lua_[key] = value;
      }
    }
  }

  void clear(const Metadata& metadata)
  {
    for (const auto& [key, value] : metadata)
    {
      lua_[key] = sol::nil;
    }
  }

  // Evaluate a pre-parsed Query. Uses the expanded Lua string.
  [[nodiscard]] bool eval(const Query& query) const noexcept
  {
    try
    {
      const auto& lua_str = query.lua();
      if (lua_str.empty()) return false;

      auto wrapped = std::string("return (") + lua_str + ")";
      auto result = lua_.safe_script(wrapped, sol::script_pass_on_error);
      if (!result.valid()) return false;

      sol::object val = result;
      return truthy(val);
    }
    catch (...)
    {
      return false;
    }
  }

  // Convenience: evaluate a raw string.
  // Tries parsing via Query for shorthand expansion. If the parser can't
  // fully handle it (raw Lua, function calls, nil comparisons), falls back
  // to passing the original string directly to Lua.
  [[nodiscard]] bool eval(std::string_view query_str) const noexcept
  {
    try
    {
      if (query_str.empty()) return false;

      Query q(query_str, {});
      auto lua_str = q.lua();

      // If the parser produced a usable expansion, use it.
      // Otherwise fall back to the raw input for complex Lua expressions.
      if (lua_str.empty()) lua_str = std::string(query_str);

      auto wrapped = std::string("return (") + lua_str + ")";
      auto result = lua_.safe_script(wrapped, sol::script_pass_on_error);
      if (!result.valid()) return false;

      sol::object val = result;
      return truthy(val);
    }
    catch (...)
    {
      return false;
    }
  }

  // Validate using Lua's parser (definitive syntax check).
  [[nodiscard]] static ValidationResult validate(std::string_view query_str) noexcept
  {
    try
    {
      if (query_str.empty()) { ValidationResult r; r.error = "empty query"; return r; }

      // Expand shorthand via Query, then check with Lua.
      // Fall back to raw input if parser can't handle it.
      Query q(query_str, {});
      auto lua_str = q.lua();
      if (lua_str.empty()) lua_str = std::string(query_str);

      sol::state tmp;
      auto wrapped = std::string("return (") + lua_str + ")";
      auto result = tmp.load(wrapped);
      if (!result.valid())
      {
        sol::error err = result;
        return parse_error(err.what());
      }
      return ValidationResult{true, {}, 0, 0};
    }
    catch (const std::exception& e)
    {
      ValidationResult r; r.error = e.what(); return r;
    }
    catch (...)
    {
      ValidationResult r; r.error = "unknown error"; return r;
    }
  }

private:
  [[nodiscard]] static std::optional<double> try_number(std::string_view s)
  {
    if (s.empty()) return std::nullopt;
    char* end = nullptr;
    double val = std::strtod(s.data(), &end);
    if (end == s.data() + s.size()) return val;
    return std::nullopt;
  }

  [[nodiscard]] static bool truthy(const sol::object& obj)
  {
    auto t = obj.get_type();
    if (t == sol::type::boolean) return obj.as<bool>();
    return t != sol::type::nil && t != sol::type::none;
  }

  [[nodiscard]] static ValidationResult parse_error(const std::string& msg)
  {
    ValidationResult r;
    r.valid = false;
    r.error = msg;

    auto colon1 = msg.find(':');
    if (colon1 == std::string::npos) return r;
    auto colon2 = msg.find(':', colon1 + 1);
    if (colon2 == std::string::npos) return r;

    auto line_str = msg.substr(colon1 + 1, colon2 - colon1 - 1);
    char* end = nullptr;
    long line = std::strtol(line_str.data(), &end, 10);
    if (end != line_str.data())
    {
      r.line = static_cast<int>(line);
    }
    return r;
  }

  mutable sol::state lua_;
};
