#include "dataload_csv.h"
#include "timestamp_parsing.h"

#include <array>
#include <set>

char DataLoadCSV::DetectDelimiter(const QString& first_line)
{
  // Count delimiters, but only outside quoted strings
  auto countOutsideQuotes = [](const QString& line, QChar delim) -> int {
    int count = 0;
    bool inside_quotes = false;
    for (const QChar& c : line)
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
    for (const QChar& c : first_line)
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

  // Find the best candidate
  // Tab and semicolon are strong indicators, comma is common, space is least reliable
  struct Candidate
  {
    char delim;
    int count;
    int priority;  // higher = preferred when counts are equal
  };

  std::array<Candidate, 4> candidates = { {
      { '\t', tab_count, 4 },       // tabs are very reliable
      { ';', semicolon_count, 3 },  // semicolons are reliable (European CSVs)
      { ',', comma_count, 2 },      // commas are common
      { ' ', space_count, 1 },      // spaces are least reliable
  } };

  Candidate* best = nullptr;
  for (auto& c : candidates)
  {
    // Space needs higher threshold since it appears in many contexts
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

void DataLoadCSV::SplitLine(const QString& line, QChar separator, QStringList& parts)
{
  parts.clear();
  bool inside_quotes = false;
  bool quoted_word = false;
  int start_pos = 0;

  int quote_start = 0;
  int quote_end = 0;

  for (int pos = 0; pos < line.size(); pos++)
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

    if ((!inside_quotes && line[pos] == separator))
    {
      part_completed = true;
    }
    if (pos + 1 == line.size())
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
      QString part;
      if (quoted_word)
      {
        part = line.mid(quote_start, quote_end - quote_start + 1);
      }
      else
      {
        part = line.mid(start_pos, end_pos - start_pos);
      }

      parts.push_back(part.trimmed());
      start_pos = pos + 1;
      quoted_word = false;
      inside_quotes = false;
    }
    if (add_empty)
    {
      parts.push_back(QString());
    }
  }
}

DataLoadCSV::DataLoadCSV()
{
  _extensions.push_back("csv");
  _delimiter = ',';
}

const std::vector<const char*>& DataLoadCSV::compatibleFileExtensions() const
{
  return _extensions;
}

void DataLoadCSV::parseHeader(QFile& file, std::vector<std::string>& column_names)
{
  file.open(QFile::ReadOnly);

  column_names.clear();

  QTextStream inA(&file);
  // The first line should contain the header. If it contains a number, we will
  // apply a name ourselves
  QString first_line = inA.readLine();

  QString preview_lines = first_line + "\n";

  QStringList firstline_items;
  SplitLine(first_line, _delimiter, firstline_items);

  int is_number_count = 0;

  std::set<std::string> different_columns;

  // check if all the elements in first row are numbers
  for (int i = 0; i < firstline_items.size(); i++)
  {
    bool isNum;
    firstline_items[i].trimmed().toDouble(&isNum);
    if (isNum)
    {
      is_number_count++;
    }
  }

  if (is_number_count == firstline_items.size())
  {
    for (int i = 0; i < firstline_items.size(); i++)
    {
      auto field_name = QString("_Column_%1").arg(i);
      auto column_name = field_name.toStdString();
      column_names.push_back(column_name);
      different_columns.insert(column_name);
    }
  }
  else
  {
    for (int i = 0; i < firstline_items.size(); i++)
    {
      // remove annoying prefix
      QString field_name(firstline_items[i].trimmed());

      if (field_name.isEmpty())
      {
        field_name = QString("_Column_%1").arg(i);
      }
      auto column_name = field_name.toStdString();
      column_names.push_back(column_name);
      different_columns.insert(column_name);
    }
  }

  if (different_columns.size() < column_names.size())
  {
    if (multiple_columns_warning_)
    {
      emit onWarningOccurred("Duplicate Column Name",
                             "Multiple Columns have the same name.\n"
                             "The column number will be added (as suffix) to the name.");
      multiple_columns_warning_ = false;
    }

    std::vector<size_t> repeated_columns;
    for (size_t i = 0; i < column_names.size(); i++)
    {
      repeated_columns.clear();
      repeated_columns.push_back(i);

      for (size_t j = i + 1; j < column_names.size(); j++)
      {
        if (column_names[i] == column_names[j])
        {
          repeated_columns.push_back(j);
        }
      }
      if (repeated_columns.size() > 1)
      {
        for (size_t index : repeated_columns)
        {
          QString suffix = "_";
          suffix += QString::number(index).rightJustified(2, '0');
          column_names[index] += suffix.toStdString();
        }
      }
    }
  }

  QStringList lines;
  for (int row = 0; row <= 100 && !inA.atEnd(); row++)
  {
    auto line = inA.readLine();
    preview_lines += line + "\n";
    lines.push_back(line);
  }

  // Update the UI widget data
  emit onParseHeader(lines, preview_lines, column_names);

  file.close();
}

// Wrapper to call the new date library-based parsing from QString
std::optional<double> AutoParseTimestamp(const QString& str)
{
  return PJ::CSV::AutoParseTimestamp(str.toStdString());
}

// Wrapper to call the new date library-based parsing from QString with format
std::optional<double> FormatParseTimestamp(const QString& str, const QString& format)
{
  return PJ::CSV::FormatParseTimestamp(str.toStdString(), format.toStdString());
}

bool DataLoadCSV::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  multiple_columns_warning_ = true;

  _fileInfo = info;

  QFile file(info->filename);
  std::vector<std::string> column_names;

  int time_index = TIME_INDEX_NOT_DEFINED;

  if (!info->plugin_config.hasChildNodes())
  {
    _default_time_axis.clear();
    // Emit here the signal that opens the dialog
    emit onLaunchDialog(file, &column_names, time_index);
  }
  else
  {
    parseHeader(file, column_names);
    if (_default_time_axis == INDEX_AS_TIME)
    {
      time_index = TIME_INDEX_GENERATED;
    }
    else
    {
      for (size_t i = 0; i < column_names.size(); i++)
      {
        if (column_names[i] == _default_time_axis)
        {
          time_index = i;
          break;
        }
      }
    }
  }

  if (time_index == TIME_INDEX_NOT_DEFINED)
  {
    return false;
  }

  //-----------------------------------
  bool interrupted = false;

  // count the number of lines first
  int tot_lines = 0;
  {
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    while (!in.atEnd())
    {
      in.readLine();
      tot_lines++;
    }
    file.close();
  }

  QProgressDialog progress_dialog;
  progress_dialog.setWindowTitle("Loading the CSV file");
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  //---- build plots_vector from header  ------

  std::vector<PlotData*> plots_vector;
  std::vector<StringSeries*> string_vector;
  bool sortRequired = false;

  for (unsigned i = 0; i < column_names.size(); i++)
  {
    const std::string& field_name = (column_names[i]);
    auto num_it = plot_data.addNumeric(field_name);
    plots_vector.push_back(&(num_it->second));

    auto str_it = plot_data.addStringSeries(field_name);
    string_vector.push_back(&(str_it->second));
  }

  //-----------------
  double prev_time = std::numeric_limits<double>::lowest();
  const QString format_string = _ui->lineEditDateFormat->text();
  const bool parse_date_format = _ui->radioCustomTime->isChecked();

  // Vector to store detected column types (detected from first data row)
  std::vector<PJ::CSV::ColumnTypeInfo> column_types(column_names.size());

  file.open(QFile::ReadOnly);
  QTextStream in(&file);
  // remove first line (header)
  QString header_str = in.readLine();
  QStringList string_items;
  QStringList header_string_items;

  SplitLine(header_str, _delimiter, header_string_items);
  QString time_header_str;
  QString t_str;
  QString prev_t_str;

  int linenumber = 1;
  int samplecount = 0;

  std::vector<std::pair<long, QString>> skipped_lines;
  bool skipped_wrong_column = false;
  bool skipped_invalid_timestamp = false;

  while (!in.atEnd())
  {
    QString line = in.readLine();
    linenumber++;
    SplitLine(line, _delimiter, string_items);

    // empty line? just try skipping
    if (string_items.size() == 0)
    {
      continue;
    }

    // corrupted line? just try skipping
    if (string_items.size() != column_names.size())
    {
      if (!skipped_wrong_column)
      {
        QMessageBox::StandardButton ret = QMessageBox::Yes;
        emit onWarningMessageBox("Unexpected column count",
                                 tr("Line %1 has %2 columns, but the expected number of "
                                    "columns is %3.\n Do you want to continue?")
                                     .arg(linenumber)
                                     .arg(string_items.size())
                                     .arg(column_names.size()),
                                 ret);

        if (ret == QMessageBox::Abort)
        {
          return false;
        }
      }
      skipped_wrong_column = true;
      skipped_lines.emplace_back(linenumber, "wrong column count");
      continue;
    }

    // identify column type, if necessary
    for (size_t i = 0; i < column_types.size(); i++)
    {
      if (column_types[i].type == PJ::CSV::ColumnType::UNDEFINED && !string_items[i].isEmpty())
      {
        column_types[i] = PJ::CSV::DetectColumnType(string_items[i].toStdString());
      }
    }

    double timestamp = samplecount;

    if (time_index >= 0)
    {
      t_str = string_items[time_index];
      const auto time_trimm = t_str.trimmed();
      bool is_number = false;

      if (parse_date_format)
      {
        if (auto ts = FormatParseTimestamp(time_trimm, format_string))
        {
          is_number = true;
          timestamp = *ts;
        }
      }
      else
      {
        // Use the detected column type for the time column
        const auto& time_type = column_types[time_index];
        if (time_type.type != PJ::CSV::ColumnType::STRING)
        {
          if (auto ts = PJ::CSV::ParseWithType(time_trimm.toStdString(), time_type))
          {
            is_number = true;
            timestamp = *ts;
          }
        }
      }

      time_header_str = header_string_items[time_index];

      if (!is_number)
      {
        if (!skipped_invalid_timestamp)
        {
          QMessageBox::StandardButton ret = QMessageBox::Yes;
          emit onWarningMessageBox("Error parsing timestamp",
                                   tr("Line %1 has an invalid timestamp: "
                                      "\"%2\".\n Do you want to continue?")
                                       .arg(linenumber)
                                       .arg(t_str),
                                   ret);

          if (ret == QMessageBox::Abort)
          {
            return false;
          }
        }
        skipped_invalid_timestamp = true;
        skipped_lines.emplace_back(linenumber, "invalid timestamp");
        continue;
      }

      if (prev_time > timestamp && !sortRequired)
      {
        QString timeName;
        timeName = time_header_str;

        const QString& title = tr("Selected time is not monotonic");
        const QString& message = tr("PlotJuggler detected that the time in this file is "
                                    "non-monotonic. This may indicate an issue with the input "
                                    "data. Continue? (Input file will not be modified but data "
                                    "will be sorted by PlotJuggler)");
        const QString& detailedText = tr("File: \"%1\" \n\n"
                                         "Selected time is not monotonic\n"
                                         "Time Index: %6 [%7]\n"
                                         "Time at line %2 : %3\n"
                                         "Time at line %4 : %5")
                                          .arg(_fileInfo->filename)
                                          .arg(linenumber - 1)
                                          .arg(prev_t_str)
                                          .arg(linenumber)
                                          .arg(t_str)
                                          .arg(time_index)
                                          .arg(timeName);

        bool sortButtonClicked = false;
        emit onWarningMessageBoxNonMonotonicTime(title, message, detailedText, sortButtonClicked);

        if (!sortButtonClicked)
        {
          return false;
        }

        sortRequired = true;
      }

      prev_time = timestamp;
      prev_t_str = t_str;
    }

    for (unsigned i = 0; i < string_items.size(); i++)
    {
      const auto& str = string_items[i];
      const auto& col_type = column_types[i];

      if (str.isEmpty() || col_type.type == PJ::CSV::ColumnType::UNDEFINED)
      {
        continue;
      }

      // Use the detected column type to parse the value
      if (col_type.type != PJ::CSV::ColumnType::STRING)
      {
        if (auto val = PJ::CSV::ParseWithType(str.toStdString(), col_type))
        {
          plots_vector[i]->pushBack({ timestamp, *val });
        }
        else
        {
          // Parsing failed - store as string
          string_vector[i]->pushBack({ timestamp, str.toStdString() });
        }
      }
      else
      {
        string_vector[i]->pushBack({ timestamp, str.toStdString() });
      }
    }

    if (linenumber % 100 == 0)
    {
      progress_dialog.setValue(linenumber);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        interrupted = true;
        break;
      }
    }
    samplecount++;
  }

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data.clear();
    return false;
  }

  if (time_index >= 0)
  {
    _default_time_axis = column_names[time_index];
  }
  else if (time_index == TIME_INDEX_GENERATED)
  {
    _default_time_axis = INDEX_AS_TIME;
  }

  // cleanups
  for (unsigned i = 0; i < column_names.size(); i++)
  {
    const auto& name = column_names[i];
    bool is_numeric = true;
    if (plots_vector[i]->size() == 0 && string_vector[i]->size() > 0)
    {
      is_numeric = false;
    }
    if (is_numeric)
    {
      plot_data.strings.erase(plot_data.strings.find(name));
    }
    else
    {
      plot_data.numeric.erase(plot_data.numeric.find(name));
    }
  }

  // Warn the user if some lines have been skipped.
  if (!skipped_lines.empty())
  {
    QString detailed_text;
    for (const auto& line : skipped_lines)
    {
      detailed_text += tr("Line %1: %2\n").arg(line.first).arg(line.second);
    }

    emit onWarningMessageBoxSkippedLines("Some lines have been skipped",
                                         tr("Some lines were not parsed as expected. "
                                            "This indicates an issue with the input data."),
                                         detailed_text);
  }

  return true;
}

bool DataLoadCSV::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("parameters");
  elem.setAttribute("time_axis", _default_time_axis.c_str());
  elem.setAttribute("delimiter", _delimiterIndex);

  QString date_format;
  if (_isCustomTime)
  {
    elem.setAttribute("date_format", _dateFormat.c_str());
  }

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCSV::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("parameters");
  if (elem.isNull())
  {
    return false;
  }
  if (elem.hasAttribute("time_axis"))
  {
    _default_time_axis = elem.attribute("time_axis").toStdString();
  }
  if (elem.hasAttribute("delimiter"))
  {
    _delimiterIndex = elem.attribute("delimiter").toInt();
    switch (_delimiterIndex)
    {
      case 0:
        _delimiter = ',';
        break;
      case 1:
        _delimiter = ';';
        break;
      case 2:
        _delimiter = ' ';
        break;
      case 3:
        _delimiter = '\t';
        break;
      default:
        _delimiter = ',';
        break;
    }
  }
  if (elem.hasAttribute("date_format"))
  {
    _dateFormat = elem.attribute("date_format").toStdString();
  }
  else
  {
    _isCustomTime = false;
  }
  return true;
}

QChar DataLoadCSV::GetDelimiter() const noexcept
{
  return _delimiter;
}

void DataLoadCSV::SetDelimiter(const char& delimiter) noexcept
{
  _delimiter = delimiter;
}

void DataLoadCSV::SetDelimiterIndex(const int delimiterIndex)
{
  _delimiterIndex = delimiterIndex;
}

int DataLoadCSV::GetDelimiterIndex() const noexcept
{
  return _delimiterIndex;
}

void DataLoadCSV::SetIsCustomTime(const bool isCustomTime)
{
  _isCustomTime = isCustomTime;
}

bool DataLoadCSV::GetIsCustomTime() const noexcept
{
  return _isCustomTime;
}

void DataLoadCSV::SetDateFormat(const std::string& dateFormat)
{
  _dateFormat = dateFormat;
}

std::string DataLoadCSV::GetDateFormat() const noexcept
{
  return _dateFormat;
}
