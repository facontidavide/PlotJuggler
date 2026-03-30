#include <gtest/gtest.h>

#include "blf_reader.h"

namespace PJ::BLF
{

TEST(BlfReaderTime, RejectsEpochLikeZeroishValuesAsWallClock)
{
  EXPECT_FALSE(IsPlausibleUnixEpochSeconds(0.0));
  EXPECT_FALSE(IsPlausibleUnixEpochSeconds(12.5));
}

TEST(BlfReaderTime, AcceptsModernWallClockEpochSeconds)
{
  EXPECT_TRUE(IsPlausibleUnixEpochSeconds(1700000000.0));
  EXPECT_TRUE(IsPlausibleUnixEpochSeconds(1800000000.0));
}

TEST(BlfReaderTime, ConvertsEpochSecondsToMsecOnlyWhenValid)
{
  bool ok = false;
  EXPECT_EQ(UnixEpochSecondsToMsec(1700000000.123, &ok), 1700000000123LL);
  EXPECT_TRUE(ok);

  EXPECT_EQ(UnixEpochSecondsToMsec(123.0, &ok), 0LL);
  EXPECT_FALSE(ok);
}

TEST(BlfReaderTime, ComputesProgressPercentageSafely)
{
  EXPECT_EQ(ComputeBlfReadPercentage(0, 0), 0);
  EXPECT_EQ(ComputeBlfReadPercentage(0, 100), 0);
  EXPECT_EQ(ComputeBlfReadPercentage(50, 100), 50);
  EXPECT_EQ(ComputeBlfReadPercentage(100, 100), 100);
  EXPECT_EQ(ComputeBlfReadPercentage(150, 100), 100);
}

}  // namespace PJ::BLF
