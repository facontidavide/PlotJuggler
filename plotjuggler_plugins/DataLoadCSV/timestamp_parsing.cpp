#include "timestamp_parsing.h"
#include <QLocale>
#include <QString>

namespace PJ::CSV
{

std::string Trim(const std::string& str)
{
  size_t start = str.find_first_not_of(" \t\r\n");
  if (start == std::string::npos)
  {
    return {};
  }
  size_t end = str.find_last_not_of(" \t\r\n");
  return str.substr(start, end - start + 1);
}

std::optional<double> toDouble(const std::string& str)
{
  // If comma is used as decimal separator (European format), replace with dot
  QString qstr = QString::fromStdString(str);
  qstr.replace(',', '.');

  // Use QLocale::c() for locale-independent parsing (always uses '.' as decimal separator)
  bool ok = false;
  double value = QLocale::c().toDouble(qstr, &ok);
  return ok ? std::optional<double>(value) : std::nullopt;
}

// Common format strings used by multiple functions
static const char* const UNAMBIGUOUS_FORMATS[] = {
  "%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M:%S%z", "%Y-%m-%dT%H:%M:%SZ",
  "%Y-%m-%d %H:%M:%S", "%Y-%m-%d %H:%M:%S%z", "%Y-%m-%d",
  "%Y/%m/%d %H:%M:%S",
};
static constexpr size_t NUM_UNAMBIGUOUS_FORMATS =
    sizeof(UNAMBIGUOUS_FORMATS) / sizeof(UNAMBIGUOUS_FORMATS[0]);

// Epoch timestamp range constants
static constexpr int64_t EPOCH_FIRST = 1400000000;  // July 14, 2014
static constexpr int64_t EPOCH_LAST = 2000000000;   // May 18, 2033

// Helper: Extract fractional seconds from a timestamp string
// Returns the base string (without fractional part) and the fractional nanoseconds
static std::pair<std::string, std::chrono::nanoseconds>
ExtractFractionalSeconds(const std::string& input)
{
  size_t dot_pos = input.rfind('.');
  size_t colon_pos = input.rfind(':');

  if (dot_pos == std::string::npos || colon_pos == std::string::npos || dot_pos <= colon_pos)
  {
    return { input, std::chrono::nanoseconds{ 0 } };
  }

  size_t frac_start = dot_pos + 1;
  size_t frac_end = frac_start;

  while (frac_end < input.size() && std::isdigit(static_cast<unsigned char>(input[frac_end])))
  {
    frac_end++;
  }

  if (frac_end <= frac_start)
  {
    return { input, std::chrono::nanoseconds{ 0 } };
  }

  std::string frac_str = input.substr(frac_start, frac_end - frac_start);

  // Pad or truncate to 9 digits (nanoseconds)
  if (frac_str.size() < 9)
  {
    frac_str.append(9 - frac_str.size(), '0');
  }
  else if (frac_str.size() > 9)
  {
    frac_str = frac_str.substr(0, 9);
  }

  std::chrono::nanoseconds fractional_ns{ 0 };
  try
  {
    fractional_ns = std::chrono::nanoseconds{ std::stoll(frac_str) };
  }
  catch (...)
  {
  }

  std::string base_str = input.substr(0, dot_pos) + input.substr(frac_end);
  return { base_str, fractional_ns };
}

// Helper: Try to parse a datetime string with a given format
static std::optional<double> TryParseFormat(const std::string& base_str, const char* fmt,
                                            std::chrono::nanoseconds fractional_ns)
{
  std::istringstream in{ base_str };
  in.imbue(std::locale::classic());

  date::sys_time<std::chrono::seconds> tp;
  in >> date::parse(fmt, tp);

  if (!in.fail())
  {
    auto duration = tp.time_since_epoch();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration) + fractional_ns;
    return std::chrono::duration<double>(total_ns).count();
  }
  return std::nullopt;
}

// Helper: Check if string is numeric and detect its type
struct NumericInfo
{
  bool is_number = false;
  bool has_decimal = false;
};

static NumericInfo CheckNumeric(const std::string& str)
{
  NumericInfo info;
  info.is_number = true;

  for (size_t i = 0; i < str.size(); i++)
  {
    char c = str[i];
    if (i == 0 && (c == '-' || c == '+'))
    {
      continue;
    }
    if (c == '.' || c == ',')
    {
      info.has_decimal = true;
      continue;
    }
    if (!std::isdigit(static_cast<unsigned char>(c)))
    {
      info.is_number = false;
      break;
    }
  }

  return info;
}

// Helper: Detect epoch timestamp type from integer value
static ColumnType DetectEpochType(int64_t ts)
{
  if (ts > EPOCH_FIRST * 1000000000LL && ts < EPOCH_LAST * 1000000000LL)
  {
    return ColumnType::EPOCH_NANOS;
  }
  if (ts > EPOCH_FIRST * 1000000LL && ts < EPOCH_LAST * 1000000LL)
  {
    return ColumnType::EPOCH_MICROS;
  }
  if (ts > EPOCH_FIRST * 1000LL && ts < EPOCH_LAST * 1000LL)
  {
    return ColumnType::EPOCH_MILLIS;
  }
  if (ts > EPOCH_FIRST && ts < EPOCH_LAST)
  {
    return ColumnType::EPOCH_SECONDS;
  }
  return ColumnType::NUMBER;
}

// Helper: Convert epoch to seconds based on type
static double EpochToSeconds(int64_t ts, ColumnType type)
{
  switch (type)
  {
    case ColumnType::EPOCH_NANOS:
      return static_cast<double>(ts) * 1e-9;
    case ColumnType::EPOCH_MICROS:
      return static_cast<double>(ts) * 1e-6;
    case ColumnType::EPOCH_MILLIS:
      return static_cast<double>(ts) * 1e-3;
    case ColumnType::EPOCH_SECONDS:
    default:
      return static_cast<double>(ts);
  }
}

bool IsDayFirstFormat(const std::string& str, char separator)
{
  int first = 0, second = 0;
  size_t pos = 0;

  while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos])))
  {
    first = first * 10 + (str[pos] - '0');
    pos++;
  }

  if (pos < str.size() && str[pos] == separator)
  {
    pos++;
  }

  while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos])))
  {
    second = second * 10 + (str[pos] - '0');
    pos++;
  }

  if (first > 12 && first <= 31)
  {
    return true;
  }
  if (second > 12 && second <= 31)
  {
    return false;
  }

  return true;  // Default to day-first
}

std::optional<double> AutoParseTimestamp(const std::string& str)
{
  std::string trimmed = Trim(str);
  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Try parsing as a number first
  auto num_info = CheckNumeric(trimmed);
  if (num_info.is_number)
  {
    try
    {
      if (!num_info.has_decimal)
      {
        int64_t ts = std::stoll(trimmed);
        ColumnType epoch_type = DetectEpochType(ts);
        return EpochToSeconds(ts, epoch_type);
      }
      else
      {
        return toDouble(trimmed);
      }
    }
    catch (...)
    {
    }
  }

  // Try datetime formats
  auto [base_str, fractional_ns] = ExtractFractionalSeconds(trimmed);

  // Try unambiguous formats first
  for (size_t i = 0; i < NUM_UNAMBIGUOUS_FORMATS; i++)
  {
    if (auto result = TryParseFormat(base_str, UNAMBIGUOUS_FORMATS[i], fractional_ns))
    {
      return result;
    }
  }

  // Try ambiguous formats with day/month detection
  if (base_str.find('/') != std::string::npos && !base_str.empty() && base_str[0] != '2')
  {
    const char* fmt = IsDayFirstFormat(base_str, '/') ? "%d/%m/%Y %H:%M:%S" : "%m/%d/%Y %H:%M:%S";
    if (auto result = TryParseFormat(base_str, fmt, fractional_ns))
    {
      return result;
    }
  }

  if (base_str.find('-') != std::string::npos && !base_str.empty() && base_str[0] != '2')
  {
    if (IsDayFirstFormat(base_str, '-'))
    {
      if (auto result = TryParseFormat(base_str, "%d-%m-%Y %H:%M:%S", fractional_ns))
      {
        return result;
      }
    }
  }

  return std::nullopt;
}

std::optional<double> FormatParseTimestamp(const std::string& str, const std::string& format)
{
  std::string trimmed = Trim(str);
  if (trimmed.empty())
  {
    return std::nullopt;
  }

  std::string adjusted_format = format;
  std::chrono::nanoseconds fractional_ns{ 0 };
  std::string adjusted_input = trimmed;

  // Handle Qt-style .zzz fractional seconds
  size_t zzz_pos = format.find(".zzz");
  if (zzz_pos != std::string::npos)
  {
    size_t z_count = 0;
    size_t z_start = zzz_pos + 1;
    while (z_start + z_count < format.size() && format[z_start + z_count] == 'z')
    {
      z_count++;
    }

    size_t input_dot_pos = adjusted_input.find('.', zzz_pos > 0 ? zzz_pos - 5 : 0);
    if (input_dot_pos != std::string::npos)
    {
      auto [base, frac] = ExtractFractionalSeconds(adjusted_input);
      fractional_ns = frac;
      adjusted_input = base;
      adjusted_format = format.substr(0, zzz_pos) + format.substr(z_start + z_count);
    }
  }

  // Convert Qt-style format to strptime format
  std::string strptime_format = adjusted_format;
  auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
      s.replace(pos, from.length(), to);
      pos += to.length();
    }
  };

  replace_all(strptime_format, "yyyy", "%Y");
  replace_all(strptime_format, "yy", "%y");
  replace_all(strptime_format, "MM", "%m");
  replace_all(strptime_format, "dd", "%d");
  replace_all(strptime_format, "hh", "%H");
  replace_all(strptime_format, "HH", "%H");
  replace_all(strptime_format, "mm", "%M");
  replace_all(strptime_format, "ss", "%S");

  return TryParseFormat(adjusted_input, strptime_format.c_str(), fractional_ns);
}

ColumnTypeInfo DetectColumnType(const std::string& str)
{
  ColumnTypeInfo info;

  std::string trimmed = Trim(str);
  if (trimmed.empty())
  {
    info.type = ColumnType::STRING;
    return info;
  }

  // Check for hexadecimal with 0x prefix (case insensitive)
  if (trimmed.size() > 2 && (trimmed[0] == '0' && (trimmed[1] == 'x' || trimmed[1] == 'X')))
  {
    info.type = ColumnType::HEX;
    return info;
  }

  auto num_info = CheckNumeric(trimmed);
  if (num_info.is_number)
  {
    if (num_info.has_decimal)
    {
      info.type = ColumnType::NUMBER;
      return info;
    }

    try
    {
      int64_t ts = std::stoll(trimmed);
      info.type = DetectEpochType(ts);
      return info;
    }
    catch (...)
    {
    }

    info.type = ColumnType::NUMBER;
    return info;
  }

  // Try datetime formats
  auto [base_str, fractional_ns] = ExtractFractionalSeconds(trimmed);
  info.has_fractional = (fractional_ns.count() > 0 || trimmed != base_str);

  // Check for separators first
  bool has_slash = base_str.find('/') != std::string::npos;
  bool has_dash = base_str.find('-') != std::string::npos;
  bool has_colon = base_str.find(':') != std::string::npos;

  // Helper lambda to try parsing with multiple formats
  auto tryParseFormats = [&base_str](const char* const* formats, size_t count,
                                     auto parse_func) -> std::optional<const char*> {
    for (size_t i = 0; i < count; i++)
    {
      std::istringstream in{ base_str };
      in.imbue(std::locale::classic());
      if (parse_func(in, formats[i]))
      {
        return formats[i];
      }
    }
    return std::nullopt;
  };

  // Check for DATE_ONLY formats FIRST (date without time)
  // Formats: "2024-01-15", "01/15/2024", "15-01-2024", "2024/01/15"
  if ((has_slash || has_dash) && !has_colon)
  {
    static const char* const date_formats[] = { "%Y-%m-%d", "%Y/%m/%d", "%d/%m/%Y",
                                                "%m/%d/%Y", "%d-%m-%Y", "%m-%d-%Y" };

    auto parse_date = [](std::istringstream& in, const char* fmt) {
      date::year_month_day ymd;
      in >> date::parse(fmt, ymd);
      return !in.fail();
    };

    if (auto fmt = tryParseFormats(date_formats, 6, parse_date))
    {
      info.type = ColumnType::DATE_ONLY;
      info.format = *fmt;
      return info;
    }
  }

  // Check for TIME_ONLY formats (time without date)
  // Formats: "14:30:25", "14:30:25.123", "2:30:25 PM"
  if (has_colon && !has_slash && !has_dash)
  {
    static const char* const time_formats[] = {
      "%H:%M:%S",
      "%I:%M:%S %p"  // 12-hour with AM/PM
    };

    auto parse_time = [](std::istringstream& in, const char* fmt) {
      std::chrono::seconds time_of_day{ 0 };
      in >> date::parse(fmt, time_of_day);
      return !in.fail();
    };

    if (auto fmt = tryParseFormats(time_formats, 2, parse_time))
    {
      info.type = ColumnType::TIME_ONLY;
      info.format = *fmt;
      info.has_fractional = (fractional_ns.count() > 0);
      return info;
    }
  }

  // Now check for full DATETIME formats (if not already detected as date-only or time-only)
  auto try_format = [&](const char* fmt) -> bool {
    std::istringstream in{ base_str };
    in.imbue(std::locale::classic());
    date::sys_time<std::chrono::seconds> tp;
    in >> date::parse(fmt, tp);
    return !in.fail();
  };

  for (size_t i = 0; i < NUM_UNAMBIGUOUS_FORMATS; i++)
  {
    if (try_format(UNAMBIGUOUS_FORMATS[i]))
    {
      info.type = ColumnType::DATETIME;
      info.format = UNAMBIGUOUS_FORMATS[i];
      return info;
    }
  }

  if (base_str.find('/') != std::string::npos && !base_str.empty() && base_str[0] != '2')
  {
    const char* fmt = IsDayFirstFormat(base_str, '/') ? "%d/%m/%Y %H:%M:%S" : "%m/%d/%Y %H:%M:%S";
    if (try_format(fmt))
    {
      info.type = ColumnType::DATETIME;
      info.format = fmt;
      return info;
    }
  }

  if (base_str.find('-') != std::string::npos && !base_str.empty() && base_str[0] != '2')
  {
    if (IsDayFirstFormat(base_str, '-'))
    {
      const char* fmt = "%d-%m-%Y %H:%M:%S";
      if (try_format(fmt))
      {
        info.type = ColumnType::DATETIME;
        info.format = fmt;
        return info;
      }
    }
  }

  info.type = ColumnType::STRING;
  return info;
}

std::optional<double> ParseWithType(const std::string& str, const ColumnTypeInfo& type_info)
{
  std::string trimmed = Trim(str);
  if (trimmed.empty())
  {
    return std::nullopt;
  }

  try
  {
    switch (type_info.type)
    {
      case ColumnType::NUMBER:
        return toDouble(trimmed);

      case ColumnType::HEX: {
        // Parse hex value with 0x prefix
        return static_cast<double>(std::stoll(trimmed, nullptr, 16));
      }

      case ColumnType::EPOCH_SECONDS:
      case ColumnType::EPOCH_MILLIS:
      case ColumnType::EPOCH_MICROS:
      case ColumnType::EPOCH_NANOS:
        return EpochToSeconds(std::stoll(trimmed), type_info.type);

      case ColumnType::DATETIME: {
        auto [base_str, fractional_ns] = ExtractFractionalSeconds(trimmed);
        if (!type_info.has_fractional)
        {
          fractional_ns = std::chrono::nanoseconds{ 0 };
        }
        return TryParseFormat(base_str, type_info.format.c_str(), fractional_ns);
      }

      case ColumnType::STRING:
      default:
        return std::nullopt;
    }
  }
  catch (...)
  {
    return std::nullopt;
  }
}

std::optional<double> ParseCombinedDateTime(const std::string& date_str,
                                            const std::string& time_str,
                                            const ColumnTypeInfo& date_info,
                                            const ColumnTypeInfo& time_info)
{
  std::string trimmed_date = Trim(date_str);
  std::string trimmed_time = Trim(time_str);

  if (trimmed_date.empty() || trimmed_time.empty())
  {
    return std::nullopt;
  }

  try
  {
    // Parse date part to get year, month, day
    std::istringstream date_in{ trimmed_date };
    date_in.imbue(std::locale::classic());

    date::year_month_day ymd;
    date_in >> date::parse(date_info.format.c_str(), ymd);

    if (date_in.fail())
    {
      return std::nullopt;
    }

    // Parse time part to get hour, minute, second
    auto [base_time_str, fractional_ns] = ExtractFractionalSeconds(trimmed_time);
    if (!time_info.has_fractional)
    {
      fractional_ns = std::chrono::nanoseconds{ 0 };
    }

    std::istringstream time_in{ base_time_str };
    time_in.imbue(std::locale::classic());

    std::chrono::seconds time_of_day{ 0 };
    time_in >> date::parse(time_info.format.c_str(), time_of_day);

    if (time_in.fail())
    {
      return std::nullopt;
    }

    // Combine date and time
    auto dp = date::sys_days{ ymd };
    auto tp = dp + time_of_day + fractional_ns;

    auto duration = tp.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
  }
  catch (...)
  {
    return std::nullopt;
  }
}

}  // namespace PJ::CSV
