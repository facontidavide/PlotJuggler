/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "core/time_format.h"

#include <iomanip>
#include <sstream>

namespace
{
struct UtcTime
{
  int month = 1;
  int day = 1;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

int64_t floorDiv(int64_t value, int64_t divisor)
{
  const auto quotient = value / divisor;
  const auto remainder = value % divisor;
  return remainder < 0 ? quotient - 1 : quotient;
}

UtcTime utcFromUnixSeconds(int64_t secs)
{
  constexpr int64_t seconds_per_day = 24 * 60 * 60;
  const int64_t days = floorDiv(secs, seconds_per_day);
  const int64_t seconds_of_day = secs - days * seconds_per_day;

  // Howard Hinnant's civil-from-days algorithm, with z as days since 1970-01-01.
  const int64_t z = days + 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int64_t doe = z - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;

  UtcTime utc;
  utc.day = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  utc.month = static_cast<int>(mp + (mp < 10 ? 3 : -9));
  utc.hour = static_cast<int>(seconds_of_day / 3600);
  utc.minute = static_cast<int>((seconds_of_day % 3600) / 60);
  utc.second = static_cast<int>(seconds_of_day % 60);
  return utc;
}

UtcTime utcFromNanoseconds(int64_t ts_ns)
{
  constexpr int64_t ns_per_second = 1'000'000'000LL;
  return utcFromUnixSeconds(floorDiv(ts_ns, ns_per_second));
}
}  // namespace

std::string formatTimestamp(int64_t ts_ns, bool long_format)
{
  const auto utc = utcFromNanoseconds(ts_ns);

  std::ostringstream os;
  if (long_format)
  {
    os << std::setfill('0') << std::setw(2) << utc.day << "/" << std::setw(2) << utc.month << " ";
  }
  os << std::setfill('0') << std::setw(2) << utc.hour << ":" << std::setw(2) << utc.minute << ":"
     << std::setw(2) << utc.second;
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
