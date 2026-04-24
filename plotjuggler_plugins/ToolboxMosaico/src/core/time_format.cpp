#include "core/time_format.h"

#include <ctime>
#include <iomanip>
#include <sstream>

std::string formatTimestamp(int64_t ts_ns, bool long_format)
{
  const auto secs = static_cast<time_t>(ts_ns / 1'000'000'000LL);
  struct tm utc{};
  gmtime_r(&secs, &utc);

  std::ostringstream os;
  if (long_format)
  {
    os << std::setfill('0') << std::setw(2) << utc.tm_mday << "/" << std::setw(2)
       << (utc.tm_mon + 1) << " ";
  }
  os << std::setfill('0') << std::setw(2) << utc.tm_hour << ":" << std::setw(2) << utc.tm_min << ":"
     << std::setw(2) << utc.tm_sec;
  return os.str();
}

std::string formatDuration(int64_t duration_ns)
{
  const int64_t total_secs = duration_ns / 1'000'000'000LL;
  if (total_secs < 60)
  {
    return std::to_string(total_secs) + "s";
  }

  const int64_t days = total_secs / 86400;
  const int64_t hours = (total_secs % 86400) / 3600;
  const int64_t minutes = (total_secs % 3600) / 60;
  const int64_t secs = total_secs % 60;

  std::string result;
  if (days > 0)
  {
    result =
        std::to_string(days) + "d " + std::to_string(hours) + "h " + std::to_string(minutes) + "m";
  }
  else if (hours > 0)
  {
    result = std::to_string(hours) + "h " + std::to_string(minutes) + "m";
  }
  else
  {
    result = std::to_string(minutes) + "m " + std::to_string(secs) + "s";
  }
  return result;
}

bool needsLongFormat(int64_t span_ns)
{
  return span_ns > 24LL * 3600 * 1'000'000'000;
}
