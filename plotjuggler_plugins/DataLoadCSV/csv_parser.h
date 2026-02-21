#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "timestamp_parsing.h"

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace PJ::CSV
{

// ---------------------------------------------------------------------------
// Low-level helpers (pure C++, no Qt)
// ---------------------------------------------------------------------------

/**
 * @brief Auto-detect the delimiter used in a CSV line.
 *
 * Analyzes the first line of a CSV file to determine the most likely delimiter.
 * Handles quoted strings correctly (delimiters inside quotes are ignored).
 *
 * @param first_line The first line of the CSV file
 * @return The detected delimiter character, or ',' as default
 */
char DetectDelimiter(const std::string& first_line);

/**
 * @brief Split a CSV line by the separator character.
 *
 * Handles quoted strings (delimiters inside quotes are preserved).
 * Trims whitespace from each part.
 *
 * @param line The CSV line to split
 * @param separator The delimiter character
 * @param parts Output: the split parts (cleared first)
 */
void SplitLine(const std::string& line, char separator, std::vector<std::string>& parts);

/**
 * @brief Parse a CSV header line into column names.
 *
 * Splits the header by delimiter, then:
 * - If all fields are numbers, generates names like "_Column_0", "_Column_1", ...
 * - Empty fields get auto-generated names
 * - Duplicate names get a suffix like "_00", "_01", ...
 *
 * @param header_line The first (header) line of the CSV
 * @param delimiter The column delimiter
 * @return A vector of unique column names
 */
std::vector<std::string> ParseHeaderLine(const std::string& header_line, char delimiter);

// ---------------------------------------------------------------------------
// Configuration and results for full CSV parsing
// ---------------------------------------------------------------------------

struct CsvParseConfig
{
  char delimiter = ',';
  int time_column_index = -1;      // -1 = use row number as time
  std::string custom_time_format;  // empty = auto-detect
  int skip_rows = 0;               // lines to skip before header
  int total_lines = 0;             // 0 = count internally if progress callback set
};

struct CsvColumnData
{
  std::string name;
  std::vector<std::pair<double, double>> numeric_points;      // (timestamp, value)
  std::vector<std::pair<double, std::string>> string_points;  // (timestamp, value)
  ColumnTypeInfo detected_type;
};

struct CsvParseWarning
{
  enum Type
  {
    WRONG_COLUMN_COUNT,
    INVALID_TIMESTAMP,
    NON_MONOTONIC_TIME,
    DUPLICATE_COLUMN_NAMES
  };
  Type type;
  int line_number;
  std::string detail;
};

struct CsvParseResult
{
  bool success = false;
  std::vector<CsvColumnData> columns;
  std::vector<std::string> column_names;
  std::vector<CsvParseWarning> warnings;
  bool time_is_non_monotonic = false;
  int lines_processed = 0;
  int lines_skipped = 0;
};

/**
 * @brief Parse CSV data from an input stream.
 *
 * Reads header, iterates data lines, detects column types, parses timestamps,
 * and accumulates results. No GUI dependency.
 *
 * @param input The input stream to read from
 * @param config Parsing configuration
 * @param progress Optional callback: progress(current_line, total_lines) → false to cancel
 * @return CsvParseResult with all parsed data, warnings, and metadata
 */
CsvParseResult ParseCsvData(std::istream& input, const CsvParseConfig& config,
                            std::function<bool(int, int)> progress = nullptr);

/**
 * @brief Parse CSV data from a string (convenience overload).
 */
CsvParseResult ParseCsvData(const std::string& csv_content, const CsvParseConfig& config,
                            std::function<bool(int, int)> progress = nullptr);

}  // namespace PJ::CSV

#endif  // CSV_PARSER_H
