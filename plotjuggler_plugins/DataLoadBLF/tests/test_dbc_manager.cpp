#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "dbc_manager.h"

namespace PJ::BLF
{

TEST(DbcManager, RejectsDuplicateChannelBinding)
{
  DbcManager manager;
  std::string error;
  const std::vector<ChannelDbcBinding> bindings = {
      {3U, "ch3_main.dbc"},
      {3U, "ch3_backup.dbc"},
  };

  EXPECT_FALSE(manager.LoadBindings(bindings, &error));
  EXPECT_NE(error.find("duplicate"), std::string::npos);
  EXPECT_NE(error.find("3"), std::string::npos);
}

}  // namespace PJ::BLF
