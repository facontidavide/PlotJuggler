#include "datetimehelp.h"
#include "dataload_csv.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QPushButton>
#include <QSyntaxStyle>
#include <QRadioButton>

#include <array>
#include <set>

#include <QStandardItemModel>

static constexpr int TIME_INDEX_NOT_DEFINED = -2;
static constexpr int TIME_INDEX_GENERATED = -1;
static constexpr const char* INDEX_AS_TIME = "__TIME_INDEX_GENERATED__";

void SplitLine(const QString& line, QChar separator, QStringList& parts)
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
  _csvHighlighter.delimiter = _delimiter;
  // setup the dialog

  _dialog = new QDialog();
  _ui = new Ui::DialogCSV();
  _ui->setupUi(_dialog);

  _dateTime_dialog = new DateTimeHelp(_dialog);

  _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  connect(_ui->radioButtonSelect, &QRadioButton::toggled, this, [this](bool checked) {
    _ui->listWidgetSeries->setEnabled(checked);
    auto selected = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
    bool box_enabled = !checked || selected.size() == 1;
    _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(box_enabled);
  });
  connect(_ui->listWidgetSeries, &QListWidget::itemSelectionChanged, this, [this]() {
    auto selected = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
    bool box_enabled = _ui->radioButtonIndex->isChecked() || selected.size() == 1;
    _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(box_enabled);
  });

  connect(_ui->listWidgetSeries, &QListWidget::itemDoubleClicked, this,
          [this]() { emit _ui->buttonBox->accepted(); });

  connect(_ui->radioCustomTime, &QRadioButton::toggled, this,
          [this](bool checked) { _ui->lineEditDateFormat->setEnabled(checked); });

  connect(_ui->dateTimeHelpButton, &QPushButton::clicked, this,
          [this]() { _dateTime_dialog->show(); });
  _ui->rawText->setHighlighter(&_csvHighlighter);

  QSizePolicy sp_retain = _ui->tableView->sizePolicy();
  sp_retain.setRetainSizeWhenHidden(true);
  _ui->tableView->setSizePolicy(sp_retain);

  _ui->splitter->setStretchFactor(0, 1);
  _ui->splitter->setStretchFactor(1, 2);

  _model = new QStandardItemModel;
  _ui->tableView->setModel(_model);
}

DataLoadCSV::~DataLoadCSV()
{
  delete _ui;
  delete _dateTime_dialog;
  delete _dialog;
}

const std::vector<const char*>& DataLoadCSV::compatibleFileExtensions() const
{
  return _extensions;
}

void DataLoadCSV::parseHeader(QFile& file, std::vector<std::string>& column_names)
{
  file.open(QFile::ReadOnly);

  _csvHighlighter.delimiter = _delimiter;

  column_names.clear();
  _ui->listWidgetSeries->clear();

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
      QMessageBox::warning(nullptr, "Duplicate Column Name",
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

  QStringList column_labels;
  for (const auto& name : column_names)
  {
    auto qname = QString::fromStdString(name);
    _ui->listWidgetSeries->addItem(qname);
    column_labels.push_back(qname);
  }
  _model->setColumnCount(column_labels.size());
  _model->setHorizontalHeaderLabels(column_labels);

  QStringList lines;

  for (int row = 0; row <= 100 && !inA.atEnd(); row++)
  {
    auto line = inA.readLine();
    preview_lines += line + "\n";
    lines.push_back(line);
  }

  _model->setRowCount(lines.count());
  for (int row = 0; row < lines.count(); row++)
  {
    QVector<QStringRef> lineToken = lines[row].splitRef(_delimiter);
    for (int j = 0; j < lineToken.size(); j++)
    {
      QString value = lineToken[j].toString();
      if (auto item = _model->item(row, j))
      {
        item->setText(value);
      }
      else
      {
        _model->setItem(row, j, new QStandardItem(value));
      }
    }
  }

  _ui->rawText->setPlainText(preview_lines);
  _ui->tableView->resizeColumnsToContents();

  file.close();
}

int DataLoadCSV::launchDialog(QFile& file, std::vector<std::string>* column_names)
{
  column_names->clear();
  _ui->tabWidget->setCurrentIndex(0);

  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadCSV.geometry").toByteArray());

  _ui->radioButtonIndex->setChecked(
      settings.value("DataLoadCSV.useIndex", false).toBool());
  bool use_custom_time = settings.value("DataLoadCSV.useDateFormat", false).toBool();
  if (use_custom_time)
  {
    _ui->radioCustomTime->setChecked(true);
  }
  else
  {
    _ui->radioAutoTime->setChecked(true);
  }
  _ui->lineEditDateFormat->setText(
      settings.value("DataLoadCSV.dateFormat", "yyyy-MM-dd hh:mm:ss").toString());

  // suggest separator
  {
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    QString first_line = in.readLine();
    int comma_count = first_line.count(QLatin1Char(','));
    int semicolon_count = first_line.count(QLatin1Char(';'));
    int space_count = first_line.count(QLatin1Char(' '));
    int tab_count = first_line.count(QLatin1Char('\t'));

    if (comma_count > 3 && comma_count > semicolon_count)
    {
      _ui->comboBox->setCurrentIndex(0);
      _delimiter = ',';
    }
    if (semicolon_count > 3 && semicolon_count > comma_count)
    {
      _ui->comboBox->setCurrentIndex(1);
      _delimiter = ';';
    }
    if (space_count > 3 && comma_count == 0 && semicolon_count == 0)
    {
      _ui->comboBox->setCurrentIndex(2);
      _delimiter = ' ';
    }
    if (tab_count > 3 && comma_count == 0 && semicolon_count == 0)
    {
      _ui->comboBox->setCurrentIndex(3);
      _delimiter = '\t';
    }
    file.close();
  }

  QString theme = settings.value("StyleSheet::theme", "light").toString();
  auto style_path = (theme == "light") ? ":/resources/lua_style_light.xml" :
                                         ":/resources/lua_style_dark.xml";

  QFile fl(style_path);
  if (fl.open(QIODevice::ReadOnly))
  {
    auto style = new QSyntaxStyle(this);
    if (style->load(fl.readAll()))
    {
      _ui->rawText->setSyntaxStyle(style);
    }
  }

  // temporary connection
  std::unique_ptr<QObject> pcontext(new QObject);
  QObject* context = pcontext.get();
  QObject::connect(_ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
                   context, [&](int index) {
                     const std::array<char, 4> delimiters = { ',', ';', ' ', '\t' };
                     _delimiter = delimiters[std::clamp(index, 0, 3)];
                     _csvHighlighter.delimiter = _delimiter;
                     parseHeader(file, *column_names);
                   });

  // parse the header once and launch the dialog
  parseHeader(file, *column_names);

  QString previous_index = settings.value("DataLoadCSV.timeIndex", "").toString();
  if (previous_index.isEmpty() == false)
  {
    auto items = _ui->listWidgetSeries->findItems(previous_index, Qt::MatchExactly);
    if (items.size() > 0)
    {
      _ui->listWidgetSeries->setCurrentItem(items.front());
    }
  }

  int res = _dialog->exec();

  settings.setValue("DataLoadCSV.geometry", _dialog->saveGeometry());
  settings.setValue("DataLoadCSV.useIndex", _ui->radioButtonIndex->isChecked());
  settings.setValue("DataLoadCSV.useDateFormat", _ui->radioCustomTime->isChecked());
  settings.setValue("DataLoadCSV.dateFormat", _ui->lineEditDateFormat->text());

  if (res == QDialog::Rejected)
  {
    return TIME_INDEX_NOT_DEFINED;
  }

  if (_ui->radioButtonIndex->isChecked())
  {
    return TIME_INDEX_GENERATED;
  }

  QModelIndexList indexes = _ui->listWidgetSeries->selectionModel()->selectedRows();
  if (indexes.size() == 1)
  {
    int row = indexes.front().row();
    auto item = _ui->listWidgetSeries->item(row);
    settings.setValue("DataLoadCSV.timeIndex", item->text());
    return row;
  }

  return TIME_INDEX_NOT_DEFINED;
}

std::optional<double> AutoParseTimestamp(const QString& str)
{
  bool is_number = false;
  QString str_trimmed = str.trimmed();
  double val = 0.0;

  // Support the case where the timestamp is in nanoseconds / microseconds
  int64_t ts = str.toLong(&is_number);
  const int64_t first_ts = 1400000000;  // July 14, 2017
  const int64_t last_ts = 2000000000;   // May 18, 2033
  if (is_number)
  {
    // check if it is an absolute time in nanoseconds.
    // convert to seconds if it is
    if (ts > first_ts * 1e9 && ts < last_ts * 1e9)
    {
      val = double(ts) * 1e-9;
    }
    else if (ts > first_ts * 1e6 && ts < last_ts * 1e6)
    {
      // check if it is an absolute time in microseconds.
      // convert to seconds if it is
      val = double(ts) * 1e-6;
    }
    else
    {
      val = double(ts);
    }
  }
  else
  {
    // Try a double value (seconds)
    val = str.toDouble(&is_number);
  }

  // handle numbers with comma instead of point as decimal separator
  if (!is_number)
  {
    static QLocale locale_with_comma(QLocale::German);
    val = locale_with_comma.toDouble(str, &is_number);
  }
  if (!is_number)
  {
    QDateTime ts = QDateTime::fromString(str, Qt::ISODateWithMs);
    if (ts.isValid())
    {
      return double(ts.toMSecsSinceEpoch()) / 1000.0;
    }
    else
    {
      return std::nullopt;
    }
  }
  return is_number ? std::optional<double>(val) : std::nullopt;
};

std::optional<double> FormatParseTimestamp(const QString& str, const QString& format)
{
  QDateTime ts = QDateTime::fromString(str, format);
  if (ts.isValid())
  {
    return double(ts.toMSecsSinceEpoch()) / 1000.0;
  }
  return std::nullopt;
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
    time_index = launchDialog(file, &column_names);
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
  int linecount = 0;

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
  progress_dialog.setRange(0, tot_lines - 1);
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

  auto ParseNumber = [&](QString str, bool& is_number) {
    QString str_trimmed = str.trimmed();
    double val = val = str_trimmed.toDouble(&is_number);
    // handle numbers with comma instead of point as decimal separator
    if (!is_number)
    {
      static QLocale locale_with_comma(QLocale::German);
      val = locale_with_comma.toDouble(str_trimmed, &is_number);
    }
    if (!is_number)
    {
      QDateTime ts;
      if (parse_date_format)
      {
        ts = QDateTime::fromString(str_trimmed, format_string);
      }
      else
      {
        ts = QDateTime::fromString(str_trimmed, Qt::ISODateWithMs);
      }
      is_number = ts.isValid();
      if (is_number)
      {
        val = ts.toMSecsSinceEpoch() / 1000.0;
      }
    }
    return val;
  };

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

  while (!in.atEnd())
  {
    QString line = in.readLine();
    SplitLine(line, _delimiter, string_items);

    // empty line? just try skipping
    if (string_items.size() == 0)
    {
      continue;
    }

    if (string_items.size() != column_names.size())
    {
      QMessageBox msgBox;
      msgBox.setWindowTitle(tr("Error reading file"));
      msgBox.setText(tr("The number of values at line %1 is %2,\n"
                        "but the expected number of columns is %3.\n"
                        "Aborting...")
                         .arg(linecount + 2)
                         .arg(string_items.size())
                         .arg(column_names.size()));

      msgBox.setDetailedText(tr("File: \"%1\" \n\n"
                                "Error reading file | Mismatched field count\n"
                                "Delimiter: [%2]\n"
                                "Header fields: %6\n"
                                "Fields on line [%4]: %7\n\n"
                                "File Preview:\n"
                                "[1]%3\n"
                                "[...]\n"
                                "[%4]%5\n")
                                 .arg(_fileInfo->filename)
                                 .arg(_delimiter)
                                 .arg(header_str)
                                 .arg(linecount + 2)
                                 .arg(line)
                                 .arg(column_names.size())
                                 .arg(string_items.size()));

      QPushButton* abortButton = msgBox.addButton(QMessageBox::Ok);

      msgBox.setIcon(QMessageBox::Warning);

      msgBox.exec();

      return false;
    }

    double timestamp = linecount;

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
        if (auto ts = AutoParseTimestamp(time_trimm))
        {
          is_number = true;
          timestamp = *ts;
        }
      }

      time_header_str = header_string_items[time_index];

      if (!is_number)
      {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Error reading file"));
        msgBox.setText(tr("Couldn't parse timestamp on line %1 with string \"%2\" . "
                          "Aborting.\n")
                           .arg(linecount + 1)
                           .arg(t_str));

        msgBox.setDetailedText(tr("File: \"%1\" \n\n"
                                  "Error reading file | Couldn't parse timestamp\n"
                                  "Parsing format: [%4]\n"
                                  "Time at line %2 : [%3]\n")
                                   .arg(_fileInfo->filename)
                                   .arg(linecount + 1)
                                   .arg(t_str)
                                   .arg((parse_date_format && !format_string.isEmpty()) ?
                                            format_string :
                                            "None"));

        QPushButton* abortButton = msgBox.addButton(QMessageBox::Ok);

        msgBox.setIcon(QMessageBox::Warning);

        msgBox.exec();

        return false;
      }

      if (prev_time > timestamp && !sortRequired)
      {
        QMessageBox msgBox;
        QString timeName;
        timeName = time_header_str;

        msgBox.setWindowTitle(tr("Selected time is not monotonic"));
        msgBox.setText(tr("PlotJuggler detected that the time in this file is "
                          "non-monotonic. This may indicate an issue with the input "
                          "data. Continue? (Input file will not be modified but data "
                          "will be sorted by PlotJuggler)"));
        msgBox.setDetailedText(tr("File: \"%1\" \n\n"
                                  "Selected time is not monotonic\n"
                                  "Time Index: %6 [%7]\n"
                                  "Time at line %2 : %3\n"
                                  "Time at line %4 : %5")
                                   .arg(_fileInfo->filename)
                                   .arg(linecount + 1)
                                   .arg(prev_t_str)
                                   .arg(linecount + 2)
                                   .arg(t_str)
                                   .arg(time_index)
                                   .arg(timeName));

        QPushButton* sortButton =
            msgBox.addButton(tr("Continue"), QMessageBox::ActionRole);
        QPushButton* abortButton = msgBox.addButton(QMessageBox::Abort);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();

        if (msgBox.clickedButton() == abortButton)
        {
          return false;
        }
        else if (msgBox.clickedButton() == sortButton)
        {
          sortRequired = true;
        }
        else
        {
          return false;
        }
      }

      prev_time = timestamp;
      prev_t_str = t_str;
    }

    for (unsigned i = 0; i < string_items.size(); i++)
    {
      bool is_number = false;
      const auto& str = string_items[i];
      double y = ParseNumber(str, is_number);
      if (is_number)
      {
        plots_vector[i]->pushBack({ timestamp, y });
      }
      else
      {
        string_vector[i]->pushBack({ timestamp, str.toStdString() });
      }
    }

    if (linecount++ % 100 == 0)
    {
      progress_dialog.setValue(linecount);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        interrupted = true;
        break;
      }
    }
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
  return true;
}

bool DataLoadCSV::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("parameters");
  elem.setAttribute("time_axis", _default_time_axis.c_str());
  elem.setAttribute("delimiter", _ui->comboBox->currentIndex());

  QString date_format;
  if (_ui->radioCustomTime->isChecked())
  {
    elem.setAttribute("date_format", _ui->lineEditDateFormat->text());
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
    int separator_index = elem.attribute("delimiter").toInt();
    _ui->comboBox->setCurrentIndex(separator_index);
    switch (separator_index)
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
    }
  }
  if (elem.hasAttribute("date_format"))
  {
    _ui->radioCustomTime->setChecked(true);
    _ui->lineEditDateFormat->setText(elem.attribute("date_format"));
  }
  else
  {
    _ui->radioAutoTime->setChecked(true);
  }
  return true;
}
