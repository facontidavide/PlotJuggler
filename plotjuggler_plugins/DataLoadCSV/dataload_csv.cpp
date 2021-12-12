#include "dataload_csv.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>

const int TIME_INDEX_NOT_DEFINED = -2;
const int TIME_INDEX_GENERATED = -1;

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
  // setup the dialog

  _dialog = new QDialog();
  _ui = new Ui::DialogCSV();
  _ui->setupUi(_dialog);

  connect(_ui->radioButtonSelect, &QRadioButton::toggled, this, [this](bool checked) {
    _ui->listWidgetSeries->setEnabled(checked);
    auto selected = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
    bool box_enabled = !checked || selected.size() == 1;
    _ui->buttonBox->setEnabled(box_enabled);
  });
  connect(_ui->listWidgetSeries, &QListWidget::itemSelectionChanged, this, [this]() {
    auto selected = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
    bool box_enabled = _ui->radioButtonIndex->isChecked() || selected.size() == 1;
    _ui->buttonBox->setEnabled(box_enabled);
  });

  connect(_ui->listWidgetSeries, &QListWidget::itemDoubleClicked, this,
          [this]() { emit _ui->buttonBox->accepted(); });

  connect(_ui->checkBoxDateFormat, &QCheckBox::toggled, this,
          [this](bool checked) { _ui->lineEditDateFormat->setEnabled(checked); });

  _ui->splitter->setStretchFactor(0, 1);
  _ui->splitter->setStretchFactor(1, 2);
}

DataLoadCSV::~DataLoadCSV()
{
  delete _ui;
  delete _dialog;
}

const std::vector<const char*>& DataLoadCSV::compatibleFileExtensions() const
{
  return _extensions;
}

void DataLoadCSV::parseHeader(QFile& file, std::vector<std::string>& column_names)
{
  file.open(QFile::ReadOnly);

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

  for (const auto& name : column_names)
  {
    _ui->listWidgetSeries->addItem(QString::fromStdString(name));
  }

  int linecount = 1;
  while (!inA.atEnd())
  {
    auto line = inA.readLine();
    if (linecount++ < 100)
    {
      preview_lines += line + "\n";
    }
    else
    {
      break;
    }
  }
  _ui->rawText->setPlainText(preview_lines);

  file.close();
}

int DataLoadCSV::launchDialog(QFile& file, std::vector<std::string>* column_names)
{
  column_names->clear();

  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadCSV.geometry").toByteArray());

  _ui->radioButtonIndex->setChecked(
      settings.value("DataLoadCSV.useIndex", false).toBool());
  _ui->checkBoxDateFormat->setChecked(
      settings.value("DataLoadCSV.useDateFormat", false).toBool());
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
    file.close();
  }

  // temporary connection
  std::unique_ptr<QObject> pcontext(new QObject);
  QObject* context = pcontext.get();
  QObject::connect(_ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged),
                   context, [&](int index) {
                     switch (index)
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
                     parseHeader(file, *column_names);
                   });

  // parse the header once and launch the dialog
  parseHeader(file, *column_names);

  int res = _dialog->exec();

  settings.setValue("DataLoadCSV.geometry", _dialog->saveGeometry());
  settings.setValue("DataLoadCSV.useIndex", _ui->radioButtonIndex->isChecked());
  settings.setValue("DataLoadCSV.useDateFormat", _ui->checkBoxDateFormat->isChecked());
  settings.setValue("DataLoadCSV.dateFormat", _ui->lineEditDateFormat->text());

  if (res == QDialog::Rejected)
  {
    return TIME_INDEX_NOT_DEFINED;
  }

  if (_ui->radioButtonIndex->isChecked())
  {
    return TIME_INDEX_GENERATED;
  }

  QModelIndexList indexes = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
  if (indexes.size() == 1)
  {
    return indexes.front().row();
  }

  return TIME_INDEX_NOT_DEFINED;
}

bool DataLoadCSV::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  bool use_provided_configuration = false;
  multiple_columns_warning_ = true;

  _default_time_axis.clear();

  if (info->plugin_config.hasChildNodes())
  {
    use_provided_configuration = true;
    xmlLoadState(info->plugin_config.firstChildElement());
  }

  QFile file(info->filename);
  std::vector<std::string> column_names;

  int time_index = TIME_INDEX_NOT_DEFINED;

  if (!use_provided_configuration)
  {
    time_index = launchDialog(file, &column_names);
  }
  else
  {
    parseHeader(file, column_names);
    if (_default_time_axis == "__TIME_INDEX_GENERATED__")
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
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, tot_lines - 1);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  //---- build plots_vector from header  ------

  std::vector<PlotData*> plots_vector;
  std::vector<StringSeries*> string_vector;

  for (unsigned i = 0; i < column_names.size(); i++)
  {
    const std::string& field_name = (column_names[i]);
    auto num_it = plot_data.addNumeric(field_name);
    plots_vector.push_back(&(num_it->second));

    auto str_it = plot_data.addStringSeries(field_name);
    string_vector.push_back(&(str_it->second));
  }

  //-----------------
  double prev_time = -std::numeric_limits<double>::max();
  bool parse_date_format = _ui->checkBoxDateFormat->isChecked();
  QString format_string = _ui->lineEditDateFormat->text();

  auto ParseNumber = [&](QString str, bool& is_number) {
    QString str_trimmed = str.trimmed();
    double val = str_trimmed.toDouble(&is_number);
    // handle numbers with comma instead of point as decimal separator
    if(!is_number)
    {
      static QLocale locale_with_comma(QLocale::German);
      val = locale_with_comma.toDouble(str_trimmed, &is_number);
    }
    if (!is_number && parse_date_format && !format_string.isEmpty())
    {
      QDateTime ts = QDateTime::fromString(str_trimmed, format_string);
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
  in.readLine();

  QStringList string_items;

  while (!in.atEnd())
  {
    QString line = in.readLine();
    SplitLine(line, _delimiter, string_items);

    if (string_items.size() != column_names.size())
    {
      auto err_msg = QString("The number of values at line %1 is %2,\n"
                             "but the expected number of columns is %3.\n"
                             "Aborting...")
                         .arg(linecount + 1)
                         .arg(string_items.size())
                         .arg(column_names.size());

      QMessageBox::warning(nullptr, "Error reading file", err_msg);
      return false;
    }

    double t = linecount;

    if (time_index >= 0)
    {
      bool is_number = false;
      QString str = string_items[time_index];
      t = ParseNumber(str, is_number);

      if (!is_number)
      {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(
            nullptr, tr("Error reading file"),
            tr("Couldn't parse timestamp with string \"%1\" . Aborting.\n").arg(str));
        return false;
      }

      if (prev_time > t)
      {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::warning(nullptr, tr("Error reading file"),
                                     tr("Selected time in not strictly monotonic. "
                                        "Loading will be aborted\n"));
        return false;
      }

      prev_time = t;
    }

    for (unsigned i = 0; i < string_items.size(); i++)
    {
      bool is_number = false;
      const auto& str = string_items[i];
      double y = ParseNumber(str, is_number);
      if (is_number)
      {
        plots_vector[i]->pushBack({ t, y });
      }
      else
      {
        string_vector[i]->pushBack({ t, str.toStdString() });
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
  }

  if (time_index >= 0)
  {
    _default_time_axis = column_names[time_index];
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
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("time_axis", _default_time_axis.c_str());
  elem.setAttribute("delimiter", _ui->comboBox->currentIndex());

  QString date_format;
  if (_ui->checkBoxDateFormat->isChecked())
  {
    elem.setAttribute("date_format", _ui->lineEditDateFormat->text());
  }

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadCSV::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
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
      _ui->checkBoxDateFormat->setChecked(true);
      _ui->lineEditDateFormat->setText(elem.attribute("date_format"));
    }
  }
  return true;
}
