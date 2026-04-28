// src/flight/logging.hpp
#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace mosaico
{

inline bool sdkLogEnabled()
{
  static const bool enabled = []() {
    const char* value = std::getenv("MOSAICO_SDK_LOG");
    if (value == nullptr)
    {
      return false;
    }
    const std::string flag(value);
    return flag == "1" || flag == "true" || flag == "TRUE" || flag == "on" || flag == "ON" ||
           flag == "debug" || flag == "DEBUG" || flag == "trace" || flag == "TRACE";
  }();
  return enabled;
}

inline void sdkLog(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::vfprintf(stderr, fmt, args);
  va_end(args);
}

}  // namespace mosaico

#define MOSAICO_SDK_LOG(...)                           \
  do                                                   \
  {                                                    \
    if (::mosaico::sdkLogEnabled())                    \
    {                                                  \
      ::mosaico::sdkLog(__VA_ARGS__);                  \
    }                                                  \
  } while (false)
