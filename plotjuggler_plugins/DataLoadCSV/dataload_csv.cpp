#include "datetimehelp.h"
#include "dataload_csv.h"
#include "csv_parser.h"

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

static constexpr int TIME_INDEX_COMBINED = -3;
static constexpr int TIME_INDEX_NOT_DEFINED = -2;
static constexpr int TIME_INDEX_GENERATED = -1;
static constexpr const char* INDEX_AS_TIME = "__TIME_INDEX_GENERATED__";

// Delegate to the pure C++ version in csv_parser
char DetectDelimiter(const QString& first_line)
{
  return PJ::CSV::DetectDelimiter(first_line.toStdString());
}

// Delegate to the pure C++ version in csv_parser
void SplitLine(const QString& line, QChar separator, QStringList& parts)
{
  std::vector<std::string> std_parts;
  PJ::CSV::SplitLine(line.toStdString(), separator.toLatin1(), std_parts);
  parts.clear();
  for (const auto& p : std_parts)
  {
    parts.push_back(QString::fromStdString(p));
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
  connect(_ui->radioButtonDateTimeColumns, &QRadioButton::toggled, this, [this](bool checked) {
    _ui->listWidgetSeries->setEnabled(!checked && _ui->radioButtonSelect->isChecked());
    if (checked)
    {
      _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
  });
  connect(_ui->listWidgetSeries, &QListWidget::itemSelectionChanged, this, [this]() {
    auto selected = _ui->listWidgetSeries->selectionModel()->selectedIndexes();
    bool box_enabled = _ui->radioButtonIndex->isChecked() ||
                       _ui->radioButtonDateTimeColumns->isChecked() || selected.size() == 1;
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

  // Delegate column name parsing to the pure C++ function
  std::string header_std = first_line.toStdString();
  std::set<std::string> before_dedup;
  {
    std::vector<std::string> raw_parts;
    PJ::CSV::SplitLine(header_std, _delimiter.toLatin1(), raw_parts);
    before_dedup.insert(raw_parts.begin(), raw_parts.end());
  }

  column_names = PJ::CSV::ParseHeaderLine(header_std, _delimiter.toLatin1());

  if (before_dedup.size() < column_names.size() && multiple_columns_warning_)
  {
    QMessageBox::warning(nullptr, "Duplicate Column Name",
                         "Multiple Columns have the same name.\n"
                         "The column number will be added (as suffix) to the name.");
    multiple_columns_warning_ = false;
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
  QStringList lineTokens;
  for (int row = 0; row < lines.count(); row++)
  {
    SplitLine(lines[row], _delimiter, lineTokens);
    for (int j = 0; j < lineTokens.size(); j++)
    {
      const QString& value = lineTokens[j];
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

  // Detect combined date+time column pairs
  _combined_columns.clear();
  _ui->radioButtonDateTimeColumns->setEnabled(false);
  _ui->radioButtonDateTimeColumns->setText(tr("Combine Date + Time columns"));

  if (!lines.isEmpty())
  {
    QStringList first_tokens;
    SplitLine(lines[0], _delimiter, first_tokens);

    std::vector<PJ::CSV::ColumnTypeInfo> col_types(column_names.size());
    for (size_t i = 0; i < col_types.size() && i < static_cast<size_t>(first_tokens.size()); i++)
    {
      if (!first_tokens[i].isEmpty())
      {
        col_types[i] = PJ::CSV::DetectColumnType(first_tokens[i].toStdString());
      }
    }

    _combined_columns = PJ::CSV::DetectCombinedDateTimeColumns(column_names, col_types);

    if (!_combined_columns.empty())
    {
      _ui->radioButtonDateTimeColumns->setEnabled(true);
      _ui->radioButtonDateTimeColumns->setText(
          tr("Combine Date + Time columns (%1)")
              .arg(QString::fromStdString(_combined_columns[0].virtual_name)));
    }
  }

  file.close();
}

int DataLoadCSV::launchDialog(QFile& file, std::vector<std::string>* column_names)
{
  column_names->clear();
  _ui->tabWidget->setCurrentIndex(0);

  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadCSV.geometry").toByteArray());

  _ui->radioButtonIndex->setChecked(settings.value("DataLoadCSV.useIndex", false).toBool());
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

  // Auto-detect delimiter from the first line
  {
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    QString first_line = in.readLine();
    file.close();

    _delimiter = DetectDelimiter(first_line);

    // Update the UI combobox to match the detected delimiter
    const std::array<char, 4> delimiters = { ',', ';', ' ', '\t' };
    for (int i = 0; i < 4; i++)
    {
      if (delimiters[i] == _delimiter)
      {
        _ui->comboBox->setCurrentIndex(i);
        break;
      }
    }
  }

  QString theme = settings.value("StyleSheet::theme", "light").toString();
  auto style_path =
      (theme == "light") ? ":/resources/lua_style_light.xml" : ":/resources/lua_style_dark.xml";

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
  QObject::connect(_ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), context,
                   [&](int index) {
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

  if (_ui->radioButtonDateTimeColumns->isChecked() && !_combined_columns.empty())
  {
    settings.setValue("DataLoadCSV.timeIndex",
                      QString::fromStdString(_combined_columns[0].virtual_name));
    return TIME_INDEX_COMBINED;
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

      // Check if the saved time axis matches a combined column pair
      if (time_index == TIME_INDEX_NOT_DEFINED)
      {
        for (size_t i = 0; i < _combined_columns.size(); i++)
        {
          if (_combined_columns[i].virtual_name == _default_time_axis)
          {
            time_index = TIME_INDEX_COMBINED;
            break;
          }
        }
      }
    }
  }

  if (time_index == TIME_INDEX_NOT_DEFINED)
  {
    return false;
  }

  //--- Build CsvParseConfig from UI state ---
  PJ::CSV::CsvParseConfig config;
  config.delimiter = _delimiter.toLatin1();
  if (time_index == TIME_INDEX_COMBINED)
  {
    config.combined_columns = _combined_columns;
    config.combined_column_index = 0;
  }
  else
  {
    config.time_column_index = time_index;
  }
  if (_ui->radioCustomTime->isChecked())
  {
    config.custom_time_format = _ui->lineEditDateFormat->text().toStdString();
  }

  //--- Count lines for progress ---
  {
    file.open(QFile::ReadOnly);
    QTextStream in(&file);
    while (!in.atEnd())
    {
      in.readLine();
      config.total_lines++;
    }
    file.close();
  }

  QProgressDialog progress_dialog;
  progress_dialog.setWindowTitle("Loading the CSV file");
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, config.total_lines);
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  //--- Parse via csv_parser ---
  bool interrupted = false;

  file.open(QFile::ReadOnly);
  QByteArray file_data = file.readAll();
  file.close();

  std::string file_str(file_data.constData(), file_data.size());

  auto result = PJ::CSV::ParseCsvData(file_str, config, [&](int current, int) -> bool {
    progress_dialog.setValue(current);
    QApplication::processEvents();
    if (progress_dialog.wasCanceled())
    {
      interrupted = true;
      return false;
    }
    return true;
  });

  if (interrupted)
  {
    progress_dialog.cancel();
    plot_data.clear();
    return false;
  }

  if (!result.success)
  {
    return false;
  }

  //--- Show non-monotonic time warning ---
  if (result.time_is_non_monotonic)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Selected time is not monotonic"));
    msgBox.setText(tr("PlotJuggler detected that the time in this file is "
                      "non-monotonic. This may indicate an issue with the input "
                      "data. Continue? (Input file will not be modified but data "
                      "will be sorted by PlotJuggler)"));

    QPushButton* sortButton = msgBox.addButton(tr("Continue"), QMessageBox::ActionRole);
    QPushButton* abortButton = msgBox.addButton(QMessageBox::Abort);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if (msgBox.clickedButton() == abortButton)
    {
      return false;
    }
    else if (msgBox.clickedButton() != sortButton)
    {
      return false;
    }
  }

  //--- Convert CsvParseResult → PlotData ---
  for (size_t i = 0; i < result.columns.size(); i++)
  {
    const auto& col = result.columns[i];
    const std::string& name = col.name;

    bool has_numeric = !col.numeric_points.empty();
    bool has_string = !col.string_points.empty();

    if (has_numeric)
    {
      auto num_it = plot_data.addNumeric(name);
      auto& series = num_it->second;
      for (const auto& [ts, val] : col.numeric_points)
      {
        series.pushBack({ ts, val });
      }
    }
    if (has_string && !has_numeric)
    {
      auto str_it = plot_data.addStringSeries(name);
      auto& series = str_it->second;
      for (const auto& [ts, val] : col.string_points)
      {
        series.pushBack({ ts, val });
      }
    }
    // If column has both numeric and string data (parse failures),
    // keep only numeric and discard string fallbacks
    if (!has_numeric && !has_string)
    {
      // Column with no data at all — still register it as numeric
      plot_data.addNumeric(name);
    }
  }

  //--- Update default time axis ---
  if (time_index == TIME_INDEX_COMBINED && !_combined_columns.empty())
  {
    _default_time_axis = _combined_columns[0].virtual_name;
  }
  else if (time_index >= 0 && time_index < static_cast<int>(result.column_names.size()))
  {
    _default_time_axis = result.column_names[time_index];
  }
  else if (time_index == TIME_INDEX_GENERATED)
  {
    _default_time_axis = INDEX_AS_TIME;
  }

  //--- Show skipped-lines warnings ---
  bool has_skipped = false;
  QString detailed_text;
  for (const auto& warn : result.warnings)
  {
    if (warn.type == PJ::CSV::CsvParseWarning::WRONG_COLUMN_COUNT ||
        warn.type == PJ::CSV::CsvParseWarning::INVALID_TIMESTAMP)
    {
      has_skipped = true;
      detailed_text +=
          tr("Line %1: %2\n").arg(warn.line_number).arg(QString::fromStdString(warn.detail));
    }
  }
  if (has_skipped)
  {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Some lines have been skipped"));
    msgBox.setText(tr("Some lines were not parsed as expected. "
                      "This indicates an issue with the input data."));
    msgBox.setDetailedText(detailed_text);
    msgBox.addButton(tr("Continue"), QMessageBox::ActionRole);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
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
      case 3:
        _delimiter = '\t';
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
