#ifndef TIMESTAMP_PARSING_H
#define TIMESTAMP_PARSING_H

#include <algorithm>
#include <optional>
#include <string>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <locale>
#include <cctype>

// Howard Hinnant's date library - header only
#include "date/date.h"

namespace PJ::CSV
{

/**
 * @brief Trim whitespace from both ends of a string.
 * @param str The string to trim
 * @return The trimmed string, or empty string if input is all whitespace
 */
std::string Trim(const std::string& str);

/**
 * @brief Determine if a date string uses day-first (European) or month-first (US) format.
 *
 * Analyzes dates like "17/04/2025" or "04/17/2025" to determine the format:
 * - If first number > 12: must be day-first (%d/%m/%Y)
 * - If second number > 12: must be month-first (%m/%d/%Y)
 * - If ambiguous (both <= 12): defaults to day-first (more common internationally)
 *
 * @param str The date string to analyze
 * @param separator The date separator character ('/' or '-')
 * @return true if day-first format, false if month-first format
 */
bool IsDayFirstFormat(const std::string& str, char separator);

/**
 * @brief Parse a timestamp string automatically, supporting multiple formats.
 *
 * Supports:
 * - Epoch timestamps (seconds, milliseconds, microseconds, nanoseconds)
 * - ISO 8601 formats (with T or space separator)
 * - European and US date formats (auto-detected)
 * - Fractional seconds (up to nanosecond precision)
 *
 * @param str The timestamp string to parse
 * @return Seconds since epoch as double, or nullopt if parsing fails
 */
std::optional<double> AutoParseTimestamp(const std::string& str);

/**
 * @brief Parse a timestamp using a specific format string.
 *
 * Uses the date library format specifiers (compatible with strptime/strftime):
 * - %Y: 4-digit year
 * - %m: 2-digit month (01-12)
 * - %d: 2-digit day (01-31)
 * - %H: 2-digit hour (00-23)
 * - %M: 2-digit minute (00-59)
 * - %S: 2-digit second (00-59)
 * - %z: UTC offset (+0200)
 *
 * Also supports Qt-style format specifiers (yyyy, MM, dd, hh, mm, ss, zzz).
 *
 * @param str The timestamp string to parse
 * @param format The format string (strptime or Qt-style)
 * @return Seconds since epoch as double, or nullopt if parsing fails
 */
std::optional<double> FormatParseTimestamp(const std::string& str, const std::string& format);

/**
 * @brief Describes the type of value detected in a column
 */
enum class ColumnType
{
  NUMBER,         // Plain numeric value
  EPOCH_SECONDS,  // Numeric epoch timestamp in seconds
  EPOCH_MILLIS,   // Numeric epoch timestamp in milliseconds
  EPOCH_MICROS,   // Numeric epoch timestamp in microseconds
  EPOCH_NANOS,    // Numeric epoch timestamp in nanoseconds
  DATETIME,       // Date/time string with detected format
  STRING,         // Non-numeric, non-datetime string
  UNDEFINED
};

/**
 * @brief Result of detecting the type and format of a column value
 */
struct ColumnTypeInfo
{
  ColumnType type = ColumnType::UNDEFINED;
  std::string format;           // For DATETIME type, the detected strptime format
  bool has_fractional = false;  // Whether fractional seconds are present
};

/**
 * @brief Detect the type and format of a value from the first data row.
 *
 * This is used to determine how to parse all subsequent values in a column.
 *
 * @param str The value from the first data row
 * @return ColumnTypeInfo describing the detected type and format
 */
ColumnTypeInfo DetectColumnType(const std::string& str);

/**
 * @brief Parse a value using a pre-detected column type.
 *
 * This is faster than AutoParseTimestamp because it uses the known format.
 *
 * @param str The value to parse
 * @param type_info The detected column type info
 * @return Parsed value as double, or nullopt if parsing fails
 */
std::optional<double> ParseWithType(const std::string& str, const ColumnTypeInfo& type_info);

}  // namespace PJ::CSV

#endif  // TIMESTAMP_PARSING_H
