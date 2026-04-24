/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "core/time_format.h"

#include <gtest/gtest.h>

namespace core
{

// --- formatTimestamp ---

TEST(TimeFormat, TimestampShortFormatUnder24h)
{
  // 2026-03-11 17:12:05 UTC in nanoseconds
  constexpr int64_t ts = 1773249125000000000LL;
  EXPECT_EQ(formatTimestamp(ts, /*long_format=*/false), "17:12:05");
}

TEST(TimeFormat, TimestampLongFormatOver24h)
{
  constexpr int64_t ts = 1773249125000000000LL;
  EXPECT_EQ(formatTimestamp(ts, /*long_format=*/true), "11/03 17:12:05");
}

// --- formatDuration ---

TEST(TimeFormat, DurationSeconds)
{
  constexpr int64_t ns = 42LL * 1'000'000'000;
  EXPECT_EQ(formatDuration(ns), "42s");
}

TEST(TimeFormat, DurationMinutesAndSeconds)
{
  constexpr int64_t ns = (12 * 60 + 30) * 1'000'000'000LL;
  EXPECT_EQ(formatDuration(ns), "12m 30s");
}

TEST(TimeFormat, DurationHoursAndMinutes)
{
  constexpr int64_t ns = (3 * 3600 + 45 * 60) * 1'000'000'000LL;
  EXPECT_EQ(formatDuration(ns), "3h 45m");
}

TEST(TimeFormat, DurationDaysAndHours)
{
  constexpr int64_t ns = (2 * 86400 + 5 * 3600 + 30 * 60) * 1'000'000'000LL;
  EXPECT_EQ(formatDuration(ns), "2d 5h 30m");
}

TEST(TimeFormat, DurationZero)
{
  EXPECT_EQ(formatDuration(0), "0s");
}

TEST(TimeFormat, DurationSubSecond)
{
  EXPECT_EQ(formatDuration(500'000'000LL), "0s");
}

TEST(TimeFormat, DurationExactMinute)
{
  constexpr int64_t ns = 60LL * 1'000'000'000;
  EXPECT_EQ(formatDuration(ns), "1m 0s");
}

TEST(TimeFormat, DurationExactHour)
{
  constexpr int64_t ns = 3600LL * 1'000'000'000;
  EXPECT_EQ(formatDuration(ns), "1h 0m");
}

TEST(TimeFormat, DurationExactDay)
{
  constexpr int64_t ns = 86400LL * 1'000'000'000;
  EXPECT_EQ(formatDuration(ns), "1d 0h 0m");
}

// --- needsLongFormat ---

TEST(TimeFormat, NeedsLongFormatTrue)
{
  constexpr int64_t span = 25LL * 3600 * 1'000'000'000;
  EXPECT_TRUE(needsLongFormat(span));
}

TEST(TimeFormat, NeedsLongFormatFalse)
{
  constexpr int64_t span = 23LL * 3600 * 1'000'000'000;
  EXPECT_FALSE(needsLongFormat(span));
}

}  // namespace core
