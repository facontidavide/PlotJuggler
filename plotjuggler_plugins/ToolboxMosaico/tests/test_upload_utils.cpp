#include <gtest/gtest.h>
#include "src/ui/upload/upload_utils.h"

TEST(UploadUtils, ResolveFreshName)
{
  EXPECT_EQ(resolveSequenceName("foo", {}), "foo");
}

TEST(UploadUtils, ResolveAppendsSuffixOnCollision)
{
  std::set<QString> existing = {"foo"};
  EXPECT_EQ(resolveSequenceName("foo", existing), "foo_1");
}

TEST(UploadUtils, ResolveSkipsTakenSuffixes)
{
  std::set<QString> existing = {"foo", "foo_1", "foo_2"};
  EXPECT_EQ(resolveSequenceName("foo", existing), "foo_3");
}

TEST(UploadUtils, AcceptsMcapExtension)
{
  EXPECT_TRUE(acceptsMcapFile("/tmp/x.mcap"));
  EXPECT_TRUE(acceptsMcapFile("/tmp/X.MCAP"));
  EXPECT_TRUE(acceptsMcapFile("/tmp/x.bag"));
  EXPECT_TRUE(acceptsMcapFile("/tmp/x.db3"));
}

TEST(UploadUtils, RejectsOtherExtensions)
{
  EXPECT_FALSE(acceptsMcapFile("/tmp/x.txt"));
  EXPECT_FALSE(acceptsMcapFile("/tmp/x"));
  EXPECT_FALSE(acceptsMcapFile(""));
}
