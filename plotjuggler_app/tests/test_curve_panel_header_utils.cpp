#include <gtest/gtest.h>

#include "curvelist_panel.h"

TEST(CurvePanelHeaderUtils, ShortensLongNamesWithMiddleStar)
{
  const QString input = "Logging2025-12-09_16-36-22-acc_1.00-jerk_2.00-SST_0.00-Cyc_0.00.blf";
  EXPECT_EQ(CurveListPanel::CompactHeaderText(input, 24), "Logging2025*Cyc_0.00.blf");
}

TEST(CurvePanelHeaderUtils, FormatsSingleAndMultiCounts)
{
  EXPECT_EQ(CurveListPanel::FormatSummaryLine({}, 24), "none");
  EXPECT_EQ(CurveListPanel::FormatSummaryLine({"demo.blf"}, 24), "demo.blf");
  EXPECT_EQ(CurveListPanel::FormatSummaryLine({"demo.blf", "other.blf"}, 24),
            "demo.blf (+1)");
}
