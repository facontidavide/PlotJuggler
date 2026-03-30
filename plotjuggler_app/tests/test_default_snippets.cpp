#include <gtest/gtest.h>

#include <QDomDocument>
#include <QFile>
#include <QString>

namespace
{

QString defaultSnippetsPath()
{
  return QString(PJ_PROJECT_SOURCE_DIR) + "/plotjuggler_app/resources/default.snippets.xml";
}

QDomElement findSnippetByName(const QDomDocument& doc, const QString& name)
{
  const auto root = doc.documentElement();
  for (auto elem = root.firstChildElement("snippet"); !elem.isNull();
       elem = elem.nextSiblingElement("snippet"))
  {
    if (elem.attribute("name") == name)
    {
      return elem;
    }
  }
  return {};
}

QDomDocument loadDefaultSnippets()
{
  QFile file(defaultSnippetsPath());
  EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << "Failed to open: " << defaultSnippetsPath();

  QDomDocument doc;
  QString parse_error;
  int parse_error_line = -1;
  EXPECT_TRUE(doc.setContent(&file, &parse_error, &parse_error_line))
      << "Failed to parse default snippets xml at line " << parse_error_line << ": "
      << parse_error.toStdString();

  return doc;
}

}  // namespace

TEST(DefaultSnippets, IncludesMovingAverageAndLowPassFilters)
{
  const auto doc = loadDefaultSnippets();

  EXPECT_FALSE(findSnippetByName(doc, "01_moving_average_single_signal").isNull());
  EXPECT_FALSE(findSnippetByName(doc, "02_first_order_low_pass_filter").isNull());
}

TEST(DefaultSnippets, NewSnippetsContainExpectedConfigVariables)
{
  const auto doc = loadDefaultSnippets();

  const auto ma_snippet = findSnippetByName(doc, "01_moving_average_single_signal");
  ASSERT_FALSE(ma_snippet.isNull());
  const auto ma_global = ma_snippet.firstChildElement("global").text();
  const auto ma_function = ma_snippet.firstChildElement("function").text();
  EXPECT_TRUE(ma_global.contains("moving_average_window"));
  EXPECT_TRUE(ma_function.contains("moving_average_window"));
  EXPECT_TRUE(ma_function.contains("15"));

  const auto lp_snippet = findSnippetByName(doc, "02_first_order_low_pass_filter");
  ASSERT_FALSE(lp_snippet.isNull());
  const auto lp_global = lp_snippet.firstChildElement("global").text();
  const auto lp_function = lp_snippet.firstChildElement("function").text();
  EXPECT_TRUE(lp_global.contains("dvx_low_pass_filter_alpha"));
  EXPECT_TRUE(lp_function.contains("prev_delta_v_filtered"));
  EXPECT_TRUE(lp_function.contains("delta_v"));
}
