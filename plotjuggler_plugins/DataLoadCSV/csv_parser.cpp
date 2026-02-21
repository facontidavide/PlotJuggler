#include "csv_parser.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>

namespace PJ::CSV
{

// ---------------------------------------------------------------------------
// DetectDelimiter
// ---------------------------------------------------------------------------

char DetectDelimiter(const std::string& first_line)
{
  auto countOutsideQuotes = [](const std::string& line, char delim) -> int {
    int count = 0;
    bool inside_quotes = false;
    for (char c : line)
    {
      if (c == '"')
      {
        inside_quotes = !inside_quotes;
      }
      else if (!inside_quotes && c == delim)
      {
        count++;
      }
    }
    return count;
  };

  int comma_count = countOutsideQuotes(first_line, ',');
  int semicolon_count = countOutsideQuotes(first_line, ';');
  int tab_count = countOutsideQuotes(first_line, '\t');

  // For space, count consecutive spaces as one delimiter
  int space_count = 0;
  {
    bool inside_quotes = false;
    bool prev_was_space = false;
    for (char c : first_line)
    {
      if (c == '"')
      {
        inside_quotes = !inside_quotes;
        prev_was_space = false;
      }
      else if (!inside_quotes && c == ' ')
      {
        if (!prev_was_space)
        {
          space_count++;
        }
        prev_was_space = true;
      }
      else
      {
        prev_was_space = false;
      }
    }
  }

  struct Candidate
  {
    char delim;
    int count;
    int priority;  // higher = preferred when counts are equal
  };

  std::array<Candidate, 4> candidates = { {
      { '\t', tab_count, 4 },
      { ';', semicolon_count, 3 },
      { ',', comma_count, 2 },
      { ' ', space_count, 1 },
  } };

  Candidate* best = nullptr;
  for (auto& c : candidates)
  {
    int threshold = (c.delim == ' ') ? 2 : 1;
    if (c.count >= threshold)
    {
      if (!best)
      {
        best = &c;
      }
      else if (c.count > best->count)
      {
        best = &c;
      }
      else if (c.count == best->count && c.priority > best->priority)
      {
        best = &c;
      }
    }
  }

  return best ? best->delim : ',';
}

// ---------------------------------------------------------------------------
// SplitLine
// ---------------------------------------------------------------------------

void SplitLine(const std::string& line, char separator, std::vector<std::string>& parts)
{
  parts.clear();
  bool inside_quotes = false;
  bool quoted_word = false;
  int start_pos = 0;

  int quote_start = 0;
  int quote_end = 0;

  for (int pos = 0; pos < static_cast<int>(line.size()); pos++)
  {
    if (line[pos] == '"')
    {
      if (inside_quotes)
      {
        quoted_word = true;
        quote_end = pos - 1;
      }
      else
      {
        quote_start = pos + 1;
      }
      inside_quotes = !inside_quotes;
    }

    bool part_completed = false;
    bool add_empty = false;
    int end_pos = pos;

    if (!inside_quotes && line[pos] == separator)
    {
      part_completed = true;
    }
    if (pos + 1 == static_cast<int>(line.size()))
    {
      part_completed = true;
      end_pos = pos + 1;
      // special case
      if (line[pos] == separator)
      {
        end_pos = pos;
        add_empty = true;
      }
    }

    if (part_completed)
    {
      std::string part;
      if (quoted_word)
      {
        part = line.substr(quote_start, quote_end - quote_start + 1);
      }
      else
      {
        part = line.substr(start_pos, end_pos - start_pos);
      }

      parts.push_back(Trim(part));
      start_pos = pos + 1;
      quoted_word = false;
      inside_quotes = false;
    }
    if (add_empty)
    {
      parts.push_back(std::string());
    }
  }
}

// ---------------------------------------------------------------------------
// ParseHeaderLine
// ---------------------------------------------------------------------------

std::vector<std::string> ParseHeaderLine(const std::string& header_line, char delimiter)
{
  std::vector<std::string> parts;
  SplitLine(header_line, delimiter, parts);

  std::vector<std::string> column_names;
  column_names.reserve(parts.size());

  // Check if all fields are numbers
  int is_number_count = 0;
  for (const auto& field : parts)
  {
    if (toDouble(Trim(field)).has_value())
    {
      is_number_count++;
    }
  }

  if (is_number_count == static_cast<int>(parts.size()))
  {
    // All numbers — generate column names
    for (size_t i = 0; i < parts.size(); i++)
    {
      column_names.push_back("_Column_" + std::to_string(i));
    }
  }
  else
  {
    for (size_t i = 0; i < parts.size(); i++)
    {
      std::string name = Trim(parts[i]);
      if (name.empty())
      {
        name = "_Column_" + std::to_string(i);
      }
      column_names.push_back(name);
    }
  }

  // Handle duplicate column names
  std::set<std::string> unique_names(column_names.begin(), column_names.end());
  if (unique_names.size() < column_names.size())
  {
    for (size_t i = 0; i < column_names.size(); i++)
    {
      std::vector<size_t> repeated;
      repeated.push_back(i);

      for (size_t j = i + 1; j < column_names.size(); j++)
      {
        if (column_names[i] == column_names[j])
        {
          repeated.push_back(j);
        }
      }
      if (repeated.size() > 1)
      {
        for (size_t index : repeated)
        {
          // Pad index to 2 digits: "_00", "_01", ...
          std::ostringstream suffix;
          suffix << "_" << std::setw(2) << std::setfill('0') << index;
          column_names[index] += suffix.str();
        }
      }
    }
  }

  return column_names;
}

// ---------------------------------------------------------------------------
// ParseCsvData
// ---------------------------------------------------------------------------

CsvParseResult ParseCsvData(std::istream& input, const CsvParseConfig& config,
                            std::function<bool(int, int)> progress)
{
  CsvParseResult result;

  // Skip rows before header
  for (int i = 0; i < config.skip_rows; i++)
  {
    std::string skip_line;
    if (!std::getline(input, skip_line))
    {
      return result;  // not enough lines
    }
  }

  // Read header
  std::string header_line;
  if (!std::getline(input, header_line))
  {
    return result;
  }

  // Strip trailing \r if present (Windows line endings)
  if (!header_line.empty() && header_line.back() == '\r')
  {
    header_line.pop_back();
  }

  result.column_names = ParseHeaderLine(header_line, config.delimiter);

  // Check for duplicate column warning (before dedup was applied)
  {
    std::vector<std::string> raw_parts;
    SplitLine(header_line, config.delimiter, raw_parts);
    std::set<std::string> unique_raw(raw_parts.begin(), raw_parts.end());
    if (unique_raw.size() < raw_parts.size())
    {
      CsvParseWarning warn;
      warn.type = CsvParseWarning::DUPLICATE_COLUMN_NAMES;
      warn.line_number = config.skip_rows + 1;
      warn.detail = "Duplicate column names detected; suffixes added";
      result.warnings.push_back(warn);
    }
  }

  const size_t num_columns = result.column_names.size();

  // Initialize columns
  result.columns.resize(num_columns);
  for (size_t i = 0; i < num_columns; i++)
  {
    result.columns[i].name = result.column_names[i];
  }

  // Column types detected from first data row
  std::vector<ColumnTypeInfo> column_types(num_columns);

  double prev_time = std::numeric_limits<double>::lowest();
  int linenumber = config.skip_rows + 1;  // header was this line
  int samplecount = 0;

  // Use caller-provided total_lines for progress, or count internally
  int total_lines = config.total_lines;
  if (progress && total_lines <= 0)
  {
    auto pos = input.tellg();
    if (pos != std::istream::pos_type(-1))
    {
      std::string tmp;
      while (std::getline(input, tmp))
      {
        total_lines++;
      }
      input.clear();
      input.seekg(pos);
    }
  }

  std::vector<std::string> parts;
  std::string line;

  while (std::getline(input, line))
  {
    linenumber++;

    // Strip trailing \r
    if (!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }

    SplitLine(line, config.delimiter, parts);

    // Empty line — skip
    if (parts.empty())
    {
      continue;
    }

    // Wrong column count — skip with warning
    if (parts.size() != num_columns)
    {
      CsvParseWarning warn;
      warn.type = CsvParseWarning::WRONG_COLUMN_COUNT;
      warn.line_number = linenumber;
      warn.detail = "Expected " + std::to_string(num_columns) + " columns, got " +
                    std::to_string(parts.size());
      result.warnings.push_back(warn);
      result.lines_skipped++;
      continue;
    }

    // Detect column types from first data row with non-empty cells
    for (size_t i = 0; i < num_columns; i++)
    {
      if (column_types[i].type == ColumnType::UNDEFINED && !parts[i].empty())
      {
        column_types[i] = DetectColumnType(parts[i]);
      }
    }

    // Determine timestamp
    double timestamp = samplecount;

    if (config.time_column_index >= 0 && config.time_column_index < static_cast<int>(num_columns))
    {
      const std::string& t_str = Trim(parts[config.time_column_index]);
      bool is_number = false;

      if (!config.custom_time_format.empty())
      {
        if (auto ts = FormatParseTimestamp(t_str, config.custom_time_format))
        {
          is_number = true;
          timestamp = *ts;
        }
      }
      else
      {
        const auto& time_type = column_types[config.time_column_index];
        if (time_type.type != ColumnType::STRING)
        {
          if (auto ts = ParseWithType(t_str, time_type))
          {
            is_number = true;
            timestamp = *ts;
          }
        }
      }

      if (!is_number)
      {
        CsvParseWarning warn;
        warn.type = CsvParseWarning::INVALID_TIMESTAMP;
        warn.line_number = linenumber;
        warn.detail = "Invalid timestamp: \"" + t_str + "\"";
        result.warnings.push_back(warn);
        result.lines_skipped++;
        continue;
      }

      // Non-monotonic time detection
      if (prev_time > timestamp && !result.time_is_non_monotonic)
      {
        result.time_is_non_monotonic = true;
        CsvParseWarning warn;
        warn.type = CsvParseWarning::NON_MONOTONIC_TIME;
        warn.line_number = linenumber;
        warn.detail = "Time is not monotonically increasing";
        result.warnings.push_back(warn);
      }

      prev_time = timestamp;
    }

    // Parse column values
    for (size_t i = 0; i < num_columns; i++)
    {
      const auto& str = parts[i];
      const auto& col_type = column_types[i];

      if (str.empty() || col_type.type == ColumnType::UNDEFINED)
      {
        continue;
      }

      if (col_type.type != ColumnType::STRING)
      {
        if (auto val = ParseWithType(str, col_type))
        {
          result.columns[i].numeric_points.emplace_back(timestamp, *val);
        }
        else
        {
          result.columns[i].string_points.emplace_back(timestamp, str);
        }
      }
      else
      {
        result.columns[i].string_points.emplace_back(timestamp, str);
      }
    }

    samplecount++;

    // Progress callback
    if (progress && (linenumber % 100 == 0))
    {
      if (!progress(linenumber, total_lines))
      {
        // Cancelled
        return result;
      }
    }
  }

  // Store detected types
  for (size_t i = 0; i < num_columns; i++)
  {
    result.columns[i].detected_type = column_types[i];
  }

  result.lines_processed = samplecount;
  result.success = true;
  return result;
}

CsvParseResult ParseCsvData(const std::string& csv_content, const CsvParseConfig& config,
                            std::function<bool(int, int)> progress)
{
  std::istringstream stream(csv_content);
  return ParseCsvData(stream, config, progress);
}

}  // namespace PJ::CSV
