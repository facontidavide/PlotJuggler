#include <gtest/gtest.h>
#include <stdexcept>

#include "blf_series_naming.h"

namespace PJ::BLF
{

TEST(BlfSeriesNaming, RawPrefixPath)
{
  const BlfFrameKey key{2, 0x18FF50E5, true};
  EXPECT_EQ(RawSeriesPrefix(key), "raw/can2/0x18FF50E5");
}

TEST(BlfSeriesNaming, RawBytePath)
{
  const BlfFrameKey key{1, 0x123, false};
  EXPECT_EQ(RawByteSeriesName(key, 0), "raw/can1/0x123/data_00");
  EXPECT_EQ(RawByteSeriesName(key, 15), "raw/can1/0x123/data_15");
  EXPECT_EQ(RawByteSeriesName(key, 63), "raw/can1/0x123/data_63");
  EXPECT_THROW(static_cast<void>(RawByteSeriesName(key, 64)), std::out_of_range);
  EXPECT_THROW(static_cast<void>(RawByteSeriesName(key, 300)), std::out_of_range);
}

TEST(BlfSeriesNaming, ToCanIdHexMasksAndWidth)
{
  EXPECT_EQ(ToCanIdHex(0xFFFFFFFF, false), "0x7FF");
  EXPECT_EQ(ToCanIdHex(0xFFFFFFFF, true), "0x1FFFFFFF");
  EXPECT_EQ(ToCanIdHex(0xA, false), "0x00A");
  EXPECT_EQ(ToCanIdHex(0xABC, true), "0x00000ABC");
}

TEST(BlfSeriesNaming, DecodedPath)
{
  EXPECT_EQ(DecodedSeriesName(3, "VehicleStatus", "Speed"),
            "dbc/can3/VehicleStatus/Speed");
}

TEST(BlfSeriesNaming, DecodedPathSanitizesSlash)
{
  EXPECT_EQ(DecodedSeriesName(3, "Vehicle/Status", "Speed/kmh"),
            "dbc/can3/Vehicle%2FStatus/Speed%2Fkmh");
}

TEST(BlfSeriesNaming, DecodedPathEscapingAvoidsTokenCollision)
{
  const auto a = DecodedSeriesName(1, "A/B", "sig");
  const auto b = DecodedSeriesName(1, "A%2FB", "sig");
  EXPECT_NE(a, b);
}

}  // namespace PJ::BLF
