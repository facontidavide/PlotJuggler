#include "csv_parser.h"
#include "timestamp_parsing.h"
#include <gtest/gtest.h>
#include <cmath>
#include <sstream>

using namespace PJ::CSV;

// ===========================================================================
// DetectDelimiter tests
// ===========================================================================

TEST(DetectDelimiter, Comma)
{
  EXPECT_EQ(DetectDelimiter("a,b,c"), ',');
}

TEST(DetectDelimiter, Semicolon)  // #479
{
  EXPECT_EQ(DetectDelimiter("a;b;c"), ';');
}

TEST(DetectDelimiter, Tab)  // #828
{
  EXPECT_EQ(DetectDelimiter("a\tb\tc"), '\t');
}

TEST(DetectDelimiter, Space)
{
  // Space needs at least 2 occurrences
  EXPECT_EQ(DetectDelimiter("a b c d"), ' ');
}

TEST(DetectDelimiter, TabOverComma)
{
  // Tab has higher priority than comma when counts are equal
  EXPECT_EQ(DetectDelimiter("a\tb,c"), '\t');
}

TEST(DetectDelimiter, CommaInsideQuotes)
{
  // Commas inside quotes should not be counted
  EXPECT_EQ(DetectDelimiter("\"a,b\"\tc\td"), '\t');
}

TEST(DetectDelimiter, DefaultComma)
{
  // No delimiters detected → default to comma
  EXPECT_EQ(DetectDelimiter("singlevalue"), ',');
}

// ===========================================================================
// SplitLine tests
// ===========================================================================

TEST(SplitLine, BasicComma)
{
  std::vector<std::string> parts;
  SplitLine("a,b,c", ',', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
}

TEST(SplitLine, CommaInQuotedField)  // #378
{
  std::vector<std::string> parts;
  SplitLine("\"a,b\",c,d", ',', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a,b");
  EXPECT_EQ(parts[1], "c");
  EXPECT_EQ(parts[2], "d");
}

TEST(SplitLine, QuotedHeader)
{
  std::vector<std::string> parts;
  SplitLine("\"field one\",\"field two\",\"field three\"", ',', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "field one");
  EXPECT_EQ(parts[1], "field two");
  EXPECT_EQ(parts[2], "field three");
}

TEST(SplitLine, TrailingSeparator)
{
  std::vector<std::string> parts;
  SplitLine("a,b,", ',', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "");
}

TEST(SplitLine, Semicolon)
{
  std::vector<std::string> parts;
  SplitLine("x;y;z", ';', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "x");
  EXPECT_EQ(parts[1], "y");
  EXPECT_EQ(parts[2], "z");
}

TEST(SplitLine, WhitespaceTrimming)
{
  std::vector<std::string> parts;
  SplitLine("  a , b , c  ", ',', parts);
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
}

// ===========================================================================
// ParseHeaderLine tests
// ===========================================================================

TEST(ParseHeaderLine, BasicHeader)
{
  auto names = ParseHeaderLine("time,x,y,z", ',');
  ASSERT_EQ(names.size(), 4u);
  EXPECT_EQ(names[0], "time");
  EXPECT_EQ(names[1], "x");
  EXPECT_EQ(names[2], "y");
  EXPECT_EQ(names[3], "z");
}

TEST(ParseHeaderLine, DuplicateColumns)  // #524
{
  auto names = ParseHeaderLine("x,y,x,y", ',');
  ASSERT_EQ(names.size(), 4u);
  // All should be unique
  std::set<std::string> unique(names.begin(), names.end());
  EXPECT_EQ(unique.size(), 4u);
  // Duplicates get suffixes
  EXPECT_EQ(names[0], "x_00");
  EXPECT_EQ(names[1], "y_01");
  EXPECT_EQ(names[2], "x_02");
  EXPECT_EQ(names[3], "y_03");
}

TEST(ParseHeaderLine, NumericFirstRow)
{
  // If all fields are numbers, generate column names
  auto names = ParseHeaderLine("1.0,2.0,3.0", ',');
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "_Column_0");
  EXPECT_EQ(names[1], "_Column_1");
  EXPECT_EQ(names[2], "_Column_2");
}

TEST(ParseHeaderLine, EmptyFields)
{
  auto names = ParseHeaderLine(",x,,z", ',');
  ASSERT_EQ(names.size(), 4u);
  EXPECT_EQ(names[0], "_Column_0");
  EXPECT_EQ(names[1], "x");
  EXPECT_EQ(names[2], "_Column_2");
  EXPECT_EQ(names[3], "z");
}

TEST(ParseHeaderLine, SemicolonDelimiter)
{
  auto names = ParseHeaderLine("a;b;c", ';');
  ASSERT_EQ(names.size(), 3u);
  EXPECT_EQ(names[0], "a");
  EXPECT_EQ(names[1], "b");
  EXPECT_EQ(names[2], "c");
}

// ===========================================================================
// ParseCsvData tests
// ===========================================================================

TEST(ParseCsvData, BasicNumeric)
{
  std::string csv = "x,y\n1.0,2.0\n3.0,4.0\n5.0,6.0\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;  // use row number

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.column_names.size(), 2u);
  EXPECT_EQ(result.column_names[0], "x");
  EXPECT_EQ(result.column_names[1], "y");

  ASSERT_EQ(result.columns.size(), 2u);
  EXPECT_EQ(result.columns[0].numeric_points.size(), 3u);
  EXPECT_EQ(result.columns[1].numeric_points.size(), 3u);

  // Check values
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].second, 3.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[2].second, 5.0);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[0].second, 2.0);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[1].second, 4.0);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[2].second, 6.0);

  EXPECT_EQ(result.lines_processed, 3);
}

TEST(ParseCsvData, GeneratedTimeIndex)
{
  std::string csv = "x,y\n10,20\n30,40\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;  // row number time

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);

  // Timestamps should be 0, 1 (row numbers)
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].first, 0.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].first, 1.0);
}

TEST(ParseCsvData, SemicolonDelimiter)  // #479
{
  std::string csv = "a;b;c\n1;2;3\n4;5;6\n";
  CsvParseConfig config;
  config.delimiter = ';';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns.size(), 3u);
  EXPECT_EQ(result.columns[0].numeric_points.size(), 2u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1.0);
  EXPECT_DOUBLE_EQ(result.columns[2].numeric_points[1].second, 6.0);
}

TEST(ParseCsvData, WrongColumnCount)  // #1110
{
  std::string csv = "x,y\n1,2\n3\n4,5\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.lines_processed, 2);
  EXPECT_EQ(result.lines_skipped, 1);

  // Check that a WRONG_COLUMN_COUNT warning was emitted
  bool found_warning = false;
  for (const auto& w : result.warnings)
  {
    if (w.type == CsvParseWarning::WRONG_COLUMN_COUNT)
    {
      found_warning = true;
      break;
    }
  }
  EXPECT_TRUE(found_warning);
}

TEST(ParseCsvData, SubMillisecondPrecision)  // #794
{
  // Nanosecond-precision epoch timestamp
  std::string csv = "time,val\n1700000000.123456789,42\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_GE(result.columns[1].numeric_points.size(), 1u);

  double ts = result.columns[1].numeric_points[0].first;
  // Should preserve at least microsecond precision
  EXPECT_NEAR(ts, 1700000000.123456789, 1e-6);
}

TEST(ParseCsvData, HexValues)  // #1135
{
  std::string csv = "val\n0xFF\n0x1A\n0x00\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns[0].numeric_points.size(), 3u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 255.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].second, 26.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[2].second, 0.0);
}

TEST(ParseCsvData, SparseFirstRow)  // #1226/#1233
{
  // First data row has empty cells — type detection should use later rows
  std::string csv = "a,b\n,2\n1,3\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  // Column 'a' should detect type from second row and parse "1"
  EXPECT_GE(result.columns[0].numeric_points.size(), 1u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1.0);
}

TEST(ParseCsvData, ISO8601AutoDetect)  // #1070
{
  std::string csv = "time,val\n2023-06-15T14:30:00Z,42\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns[1].numeric_points.size(), 1u);

  double ts = result.columns[1].numeric_points[0].first;
  // Should be a valid epoch timestamp (June 15, 2023 14:30 UTC)
  EXPECT_GT(ts, 1686800000.0);
  EXPECT_LT(ts, 1686900000.0);
}

TEST(ParseCsvData, CustomDateFormat)  // #906
{
  std::string csv = "time,val\n15/06/2023 14:30:00,42\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;
  config.custom_time_format = "%d/%m/%Y %H:%M:%S";

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns[1].numeric_points.size(), 1u);

  double ts = result.columns[1].numeric_points[0].first;
  EXPECT_GT(ts, 1686800000.0);
  EXPECT_LT(ts, 1686900000.0);
}

TEST(ParseCsvData, NonMonotonicTime)
{
  std::string csv = "time,val\n1.0,10\n3.0,30\n2.0,20\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_TRUE(result.time_is_non_monotonic);

  // Data should still be returned (not discarded)
  EXPECT_EQ(result.columns[1].numeric_points.size(), 3u);
}

TEST(ParseCsvData, SkipRows)  // #734/#850
{
  // Metadata lines before header
  std::string csv = "# comment line 1\n# comment line 2\ntime,val\n1.0,42\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;
  config.skip_rows = 2;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.column_names.size(), 2u);
  EXPECT_EQ(result.column_names[0], "time");
  EXPECT_EQ(result.column_names[1], "val");
  EXPECT_EQ(result.columns[1].numeric_points.size(), 1u);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[0].second, 42.0);
}

TEST(ParseCsvData, EmptyLines)
{
  std::string csv = "x,y\n1,2\n\n3,4\n\n5,6\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  // Empty lines should be skipped
  EXPECT_EQ(result.lines_processed, 3);
}

TEST(ParseCsvData, DecimalSeparator)  // #1237
{
  // European CSV: semicolon delimiter, comma decimal separator
  std::string csv = "a;b\n1,5;2,3\n4,0;5,7\n";
  CsvParseConfig config;
  config.delimiter = ';';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns[0].numeric_points.size(), 2u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1.5);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].second, 4.0);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[0].second, 2.3);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[1].second, 5.7);
}

TEST(ParseCsvData, TimeColumnAsIndex)
{
  // Use second column as time axis
  std::string csv = "val,time\n42,1.0\n84,2.0\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);

  // First column should use timestamps from second column
  ASSERT_EQ(result.columns[0].numeric_points.size(), 2u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].first, 1.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].first, 2.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 42.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].second, 84.0);
}

TEST(ParseCsvData, StringColumn)
{
  std::string csv = "name,val\nhello,1.0\nworld,2.0\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);

  // 'name' column should be detected as string
  EXPECT_EQ(result.columns[0].string_points.size(), 2u);
  EXPECT_EQ(result.columns[0].numeric_points.size(), 0u);
  EXPECT_EQ(result.columns[0].string_points[0].second, "hello");
  EXPECT_EQ(result.columns[0].string_points[1].second, "world");

  // 'val' column should be numeric
  EXPECT_EQ(result.columns[1].numeric_points.size(), 2u);
  EXPECT_EQ(result.columns[1].string_points.size(), 0u);
}

TEST(ParseCsvData, WindowsLineEndings)
{
  std::string csv = "x,y\r\n1,2\r\n3,4\r\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.lines_processed, 2);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1.0);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[1].second, 4.0);
}

TEST(ParseCsvData, InvalidTimestamp)
{
  std::string csv = "time,val\n1.0,10\nnot_a_time,20\n3.0,30\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.lines_processed, 2);
  EXPECT_EQ(result.lines_skipped, 1);

  bool found_warning = false;
  for (const auto& w : result.warnings)
  {
    if (w.type == CsvParseWarning::INVALID_TIMESTAMP)
    {
      found_warning = true;
      break;
    }
  }
  EXPECT_TRUE(found_warning);
}

TEST(ParseCsvData, ProgressCallback)
{
  // Build a CSV with enough lines to trigger progress
  std::string csv = "x\n";
  for (int i = 0; i < 250; i++)
  {
    csv += std::to_string(i) + "\n";
  }

  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  int progress_calls = 0;
  auto result = ParseCsvData(csv, config, [&](int, int) -> bool {
    progress_calls++;
    return true;
  });

  ASSERT_TRUE(result.success);
  EXPECT_GT(progress_calls, 0);
  EXPECT_EQ(result.lines_processed, 250);
}

TEST(ParseCsvData, ProgressCancellation)
{
  std::string csv = "x\n";
  for (int i = 0; i < 250; i++)
  {
    csv += std::to_string(i) + "\n";
  }

  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config, [&](int, int) -> bool {
    return false;  // cancel immediately
  });

  // Should have stopped early
  EXPECT_FALSE(result.success);
  EXPECT_LT(result.lines_processed, 250);
}

TEST(ParseCsvData, EmptyInput)
{
  std::string csv = "";
  CsvParseConfig config;
  config.delimiter = ',';

  auto result = ParseCsvData(csv, config);
  EXPECT_FALSE(result.success);
}

TEST(ParseCsvData, HeaderOnly)
{
  std::string csv = "x,y,z\n";
  CsvParseConfig config;
  config.delimiter = ',';

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.column_names.size(), 3u);
  EXPECT_EQ(result.lines_processed, 0);
}

TEST(ParseCsvData, ScientificNotation)  // PR #1280
{
  std::string csv = "val\n1.5e3\n2.0E-4\n-3e2\n1e10\n";
  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = -1;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns.size(), 1u);

  // All values should be parsed as numeric, not string
  EXPECT_EQ(result.columns[0].string_points.size(), 0u);
  ASSERT_EQ(result.columns[0].numeric_points.size(), 4u);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[0].second, 1500.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[1].second, 0.0002);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[2].second, -300.0);
  EXPECT_DOUBLE_EQ(result.columns[0].numeric_points[3].second, 1e10);
}

// ===========================================================================
// DetectColumnType — DATE_ONLY / TIME_ONLY tests
// ===========================================================================

TEST(DetectColumnType, DateOnlyISO)
{
  auto info = DetectColumnType("2024-01-15");
  EXPECT_EQ(info.type, ColumnType::DATE_ONLY);
  EXPECT_EQ(info.format, "%Y-%m-%d");
}

TEST(DetectColumnType, DateOnlySlash)
{
  auto info = DetectColumnType("2024/06/15");
  EXPECT_EQ(info.type, ColumnType::DATE_ONLY);
  EXPECT_EQ(info.format, "%Y/%m/%d");
}

TEST(DetectColumnType, DateOnlyDayFirst)
{
  auto info = DetectColumnType("15/06/2024");
  EXPECT_EQ(info.type, ColumnType::DATE_ONLY);
  // 15 > 12, so unambiguously day-first
  EXPECT_EQ(info.format, "%d/%m/%Y");
}

TEST(DetectColumnType, TimeOnlyHMS)
{
  auto info = DetectColumnType("14:30:25");
  EXPECT_EQ(info.type, ColumnType::TIME_ONLY);
  EXPECT_EQ(info.format, "%H:%M:%S");
}

TEST(DetectColumnType, TimeOnlyWithFractional)
{
  auto info = DetectColumnType("14:30:25.123");
  EXPECT_EQ(info.type, ColumnType::TIME_ONLY);
  EXPECT_EQ(info.format, "%H:%M:%S");
  EXPECT_TRUE(info.has_fractional);
}

TEST(DetectColumnType, DateTimeStillDetected)
{
  // Full datetime should still be DATETIME, not DATE_ONLY
  auto info = DetectColumnType("2024-01-15T14:30:25");
  EXPECT_EQ(info.type, ColumnType::DATETIME);
}

// ===========================================================================
// ParseWithType — DATE_ONLY / TIME_ONLY tests
// ===========================================================================

TEST(ParseWithType, DateOnlyReturnsEpoch)
{
  ColumnTypeInfo info;
  info.type = ColumnType::DATE_ONLY;
  info.format = "%Y-%m-%d";

  auto result = ParseWithType("2024-01-15", info);
  ASSERT_TRUE(result.has_value());
  // 2024-01-15 midnight UTC = 1705276800.0
  EXPECT_NEAR(*result, 1705276800.0, 1.0);
}

TEST(ParseWithType, TimeOnlyReturnsSecondsFromMidnight)
{
  ColumnTypeInfo info;
  info.type = ColumnType::TIME_ONLY;
  info.format = "%H:%M:%S";

  auto result = ParseWithType("14:30:25", info);
  ASSERT_TRUE(result.has_value());
  // 14*3600 + 30*60 + 25 = 52225
  EXPECT_DOUBLE_EQ(*result, 52225.0);
}

TEST(ParseWithType, TimeOnlyWithFractionalSeconds)
{
  ColumnTypeInfo info;
  info.type = ColumnType::TIME_ONLY;
  info.format = "%H:%M:%S";
  info.has_fractional = true;

  auto result = ParseWithType("10:30:25.500", info);
  ASSERT_TRUE(result.has_value());
  // 10*3600 + 30*60 + 25.5 = 37825.5
  EXPECT_NEAR(*result, 37825.5, 1e-3);
}

// ===========================================================================
// ParseCombinedDateTime tests
// ===========================================================================

TEST(ParseCombinedDateTime, BasicCombination)
{
  ColumnTypeInfo date_info;
  date_info.type = ColumnType::DATE_ONLY;
  date_info.format = "%Y-%m-%d";

  ColumnTypeInfo time_info;
  time_info.type = ColumnType::TIME_ONLY;
  time_info.format = "%H:%M:%S";

  auto result = ParseCombinedDateTime("2024-01-15", "14:30:25", date_info, time_info);
  ASSERT_TRUE(result.has_value());
  // 2024-01-15 14:30:25 UTC = 1705329025.0
  EXPECT_NEAR(*result, 1705329025.0, 1.0);
}

TEST(ParseCombinedDateTime, WithFractionalSeconds)
{
  ColumnTypeInfo date_info;
  date_info.type = ColumnType::DATE_ONLY;
  date_info.format = "%Y-%m-%d";

  ColumnTypeInfo time_info;
  time_info.type = ColumnType::TIME_ONLY;
  time_info.format = "%H:%M:%S";
  time_info.has_fractional = true;

  auto r1 = ParseCombinedDateTime("2024-01-15", "14:30:25.000", date_info, time_info);
  auto r2 = ParseCombinedDateTime("2024-01-15", "14:30:25.500", date_info, time_info);
  ASSERT_TRUE(r1.has_value());
  ASSERT_TRUE(r2.has_value());
  EXPECT_NEAR(*r2 - *r1, 0.5, 1e-3);
}

TEST(ParseCombinedDateTime, InvalidDateReturnsNullopt)
{
  ColumnTypeInfo date_info;
  date_info.type = ColumnType::DATE_ONLY;
  date_info.format = "%Y-%m-%d";

  ColumnTypeInfo time_info;
  time_info.type = ColumnType::TIME_ONLY;
  time_info.format = "%H:%M:%S";

  auto result = ParseCombinedDateTime("not-a-date", "14:30:25", date_info, time_info);
  EXPECT_FALSE(result.has_value());
}

TEST(ParseCombinedDateTime, InvalidTimeReturnsNullopt)
{
  ColumnTypeInfo date_info;
  date_info.type = ColumnType::DATE_ONLY;
  date_info.format = "%Y-%m-%d";

  ColumnTypeInfo time_info;
  time_info.type = ColumnType::TIME_ONLY;
  time_info.format = "%H:%M:%S";

  auto result = ParseCombinedDateTime("2024-01-15", "bad-time", date_info, time_info);
  EXPECT_FALSE(result.has_value());
}

TEST(ParseCombinedDateTime, EmptyStringsReturnNullopt)
{
  ColumnTypeInfo date_info;
  date_info.type = ColumnType::DATE_ONLY;
  date_info.format = "%Y-%m-%d";

  ColumnTypeInfo time_info;
  time_info.type = ColumnType::TIME_ONLY;
  time_info.format = "%H:%M:%S";

  EXPECT_FALSE(ParseCombinedDateTime("", "14:30:25", date_info, time_info).has_value());
  EXPECT_FALSE(ParseCombinedDateTime("2024-01-15", "", date_info, time_info).has_value());
}

// ===========================================================================
// DetectCombinedDateTimeColumns tests
// ===========================================================================

TEST(DetectCombinedDateTimeColumns, AdjacentPair)
{
  std::vector<std::string> names = { "Date", "Time", "Value" };
  std::vector<ColumnTypeInfo> types(3);
  types[0].type = ColumnType::DATE_ONLY;
  types[0].format = "%Y-%m-%d";
  types[1].type = ColumnType::TIME_ONLY;
  types[1].format = "%H:%M:%S";
  types[2].type = ColumnType::NUMBER;

  auto pairs = DetectCombinedDateTimeColumns(names, types);
  ASSERT_EQ(pairs.size(), 1u);
  EXPECT_EQ(pairs[0].date_column_index, 0);
  EXPECT_EQ(pairs[0].time_column_index, 1);
  EXPECT_EQ(pairs[0].virtual_name, "Date + Time");
}

TEST(DetectCombinedDateTimeColumns, ReversedOrder)
{
  std::vector<std::string> names = { "Time", "Date", "Value" };
  std::vector<ColumnTypeInfo> types(3);
  types[0].type = ColumnType::TIME_ONLY;
  types[0].format = "%H:%M:%S";
  types[1].type = ColumnType::DATE_ONLY;
  types[1].format = "%Y-%m-%d";
  types[2].type = ColumnType::NUMBER;

  auto pairs = DetectCombinedDateTimeColumns(names, types);
  ASSERT_EQ(pairs.size(), 1u);
  EXPECT_EQ(pairs[0].date_column_index, 1);
  EXPECT_EQ(pairs[0].time_column_index, 0);
  EXPECT_EQ(pairs[0].virtual_name, "Date + Time");
}

TEST(DetectCombinedDateTimeColumns, NonAdjacentNoPair)
{
  std::vector<std::string> names = { "Date", "Value", "Time" };
  std::vector<ColumnTypeInfo> types(3);
  types[0].type = ColumnType::DATE_ONLY;
  types[1].type = ColumnType::NUMBER;
  types[2].type = ColumnType::TIME_ONLY;

  auto pairs = DetectCombinedDateTimeColumns(names, types);
  EXPECT_TRUE(pairs.empty());
}

TEST(DetectCombinedDateTimeColumns, MultiplePairs)
{
  std::vector<std::string> names = { "Date1", "Time1", "Date2", "Time2" };
  std::vector<ColumnTypeInfo> types(4);
  types[0].type = ColumnType::DATE_ONLY;
  types[0].format = "%Y-%m-%d";
  types[1].type = ColumnType::TIME_ONLY;
  types[1].format = "%H:%M:%S";
  types[2].type = ColumnType::DATE_ONLY;
  types[2].format = "%Y-%m-%d";
  types[3].type = ColumnType::TIME_ONLY;
  types[3].format = "%H:%M:%S";

  auto pairs = DetectCombinedDateTimeColumns(names, types);
  ASSERT_EQ(pairs.size(), 2u);
  EXPECT_EQ(pairs[0].date_column_index, 0);
  EXPECT_EQ(pairs[0].time_column_index, 1);
  EXPECT_EQ(pairs[1].date_column_index, 2);
  EXPECT_EQ(pairs[1].time_column_index, 3);
}

// ===========================================================================
// End-to-end: ParseCsvData with combined columns
// ===========================================================================

TEST(ParseCsvData, CombinedDateTimeColumns)
{
  std::string csv = "Date,Time,Temperature\n"
                    "2024-01-15,10:30:25.000,23.5\n"
                    "2024-01-15,10:30:26.000,23.6\n"
                    "2024-01-15,10:30:27.000,23.7\n";

  CsvParseConfig config;
  config.delimiter = ',';

  // Set up combined columns
  CombinedColumnPair combo;
  combo.date_column_index = 0;
  combo.time_column_index = 1;
  combo.virtual_name = "Date + Time";
  config.combined_columns.push_back(combo);
  config.combined_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  ASSERT_EQ(result.columns.size(), 3u);

  // Temperature column (index 2) should have 3 data points
  ASSERT_EQ(result.columns[2].numeric_points.size(), 3u);
  EXPECT_DOUBLE_EQ(result.columns[2].numeric_points[0].second, 23.5);
  EXPECT_DOUBLE_EQ(result.columns[2].numeric_points[1].second, 23.6);
  EXPECT_DOUBLE_EQ(result.columns[2].numeric_points[2].second, 23.7);

  // Timestamps should be 1 second apart
  double ts0 = result.columns[2].numeric_points[0].first;
  double ts1 = result.columns[2].numeric_points[1].first;
  double ts2 = result.columns[2].numeric_points[2].first;
  EXPECT_NEAR(ts1 - ts0, 1.0, 1e-3);
  EXPECT_NEAR(ts2 - ts1, 1.0, 1e-3);

  // Combined component columns should be tracked
  EXPECT_TRUE(result.combined_component_indices.count(0) > 0);
  EXPECT_TRUE(result.combined_component_indices.count(1) > 0);

  // Date and Time columns should have been skipped (no data points)
  EXPECT_EQ(result.columns[0].numeric_points.size(), 0u);
  EXPECT_EQ(result.columns[0].string_points.size(), 0u);
  EXPECT_EQ(result.columns[1].numeric_points.size(), 0u);
  EXPECT_EQ(result.columns[1].string_points.size(), 0u);
}

TEST(ParseCsvData, CombinedDateTimeWithInvalidRow)
{
  std::string csv = "Date,Time,Value\n"
                    "2024-01-15,10:30:25,100\n"
                    "bad-date,10:30:26,200\n"
                    "2024-01-15,10:30:27,300\n";

  CsvParseConfig config;
  config.delimiter = ',';

  CombinedColumnPair combo;
  combo.date_column_index = 0;
  combo.time_column_index = 1;
  combo.virtual_name = "Date + Time";
  config.combined_columns.push_back(combo);
  config.combined_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);

  // 2 valid lines, 1 skipped
  EXPECT_EQ(result.lines_processed, 2);
  EXPECT_EQ(result.lines_skipped, 1);

  // Check that an INVALID_TIMESTAMP warning was emitted
  bool found_warning = false;
  for (const auto& w : result.warnings)
  {
    if (w.type == CsvParseWarning::INVALID_TIMESTAMP)
    {
      found_warning = true;
      break;
    }
  }
  EXPECT_TRUE(found_warning);
}

// ===========================================================================
// Timestamp column starting with "0"
// ===========================================================================

TEST(DetectColumnType, ZeroIsNumber)
{
  auto info = DetectColumnType("0");
  EXPECT_EQ(info.type, ColumnType::NUMBER);
}

TEST(ParseCsvData, TimestampColumnStartingAtZero)
{
  std::string csv = "time,val\n"
                    "0,10\n"
                    "0.003,20\n"
                    "0.005,30\n";

  CsvParseConfig config;
  config.delimiter = ',';
  config.time_column_index = 0;

  auto result = ParseCsvData(csv, config);
  ASSERT_TRUE(result.success);
  EXPECT_EQ(result.lines_processed, 3);
  EXPECT_EQ(result.lines_skipped, 0);
  EXPECT_EQ(result.columns[0].detected_type.type, ColumnType::NUMBER);

  ASSERT_EQ(result.columns[1].numeric_points.size(), 3u);
  EXPECT_DOUBLE_EQ(result.columns[1].numeric_points[0].first, 0.0);
  EXPECT_NEAR(result.columns[1].numeric_points[1].first, 0.003, 1e-9);
  EXPECT_NEAR(result.columns[1].numeric_points[2].first, 0.005, 1e-9);
}
