#include <gtest/gtest.h>

#include <type_traits>

#include "PlotJuggler/util/delayed_callback.hpp"
#include "plugin_manager.h"

TEST(PluginManagerUtils, UsesSameDedupKeyForSameLibraryNameAcrossFolders)
{
  const QString build_path = "/tmp/build/libToolboxLuaEditor.so";
  const QString install_path = "/opt/app/lib/plotjuggler/plugins/libToolboxLuaEditor.so";

  EXPECT_EQ(PJ::PluginDedupKey(build_path), PJ::PluginDedupKey(install_path));
}

TEST(PluginManagerUtils, UsesDifferentDedupKeyForDifferentLibraries)
{
  EXPECT_NE(PJ::PluginDedupKey("/tmp/build/libToolboxLuaEditor.so"),
            PJ::PluginDedupKey("/tmp/build/libToolboxFFT.so"));
}

TEST(PluginManagerUtils, DelayedCallbackIsNotCopyable)
{
  EXPECT_FALSE(std::is_copy_constructible<PJ::DelayedCallback>::value);
  EXPECT_FALSE(std::is_copy_assignable<PJ::DelayedCallback>::value);
}
