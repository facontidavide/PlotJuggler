//
// Created by aGendy on 06/01/2026.
//

#include "dataload_csv_ui.h"
#include "datetimehelp.h"
#include "dataload_csv.h"
#include <array>

// Qt Deps
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
#include <QStandardItemModel>

DataLoadCSVUI::DataLoadCSVUI()
{
  _dataLoadCSV = std::make_unique<DataLoadCSV>();
  _csvHighlighter.delimiter = _dataLoadCSV->GetDelimiter();

  // set up the dialog
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

  connect(_dataLoadCSV.get(), &DataLoadCSV::warningOccurred, this,
          &DataLoadCSVUI::ShowWarningMessage);

  connect(_dataLoadCSV.get(), &DataLoadCSV::onParseHeader, this, &DataLoadCSVUI::ParseHeader);

  _ui->rawText->setHighlighter(&_csvHighlighter);

  QSizePolicy sp_retain = _ui->tableView->sizePolicy();
  sp_retain.setRetainSizeWhenHidden(true);
  _ui->tableView->setSizePolicy(sp_retain);

  _ui->splitter->setStretchFactor(0, 1);
  _ui->splitter->setStretchFactor(1, 2);

  _model = new QStandardItemModel;
  _ui->tableView->setModel(_model);
}

DataLoadCSVUI::~DataLoadCSVUI()
{
  delete _ui;
  delete _dateTime_dialog;
  delete _dialog;
}

void DataLoadCSVUI::ParseHeader(const QStringList& lines, const QString& preview_lines,
                                const std::vector<std::string>& column_names)
{
  _csvHighlighter.delimiter = _dataLoadCSV->GetDelimiter();
  _ui->listWidgetSeries->clear();

  QStringList column_labels;
  for (const auto& name : column_names)
  {
    auto qname = QString::fromStdString(name);
    _ui->listWidgetSeries->addItem(qname);
    column_labels.push_back(qname);
  }
  _model->setColumnCount(column_labels.size());
  _model->setHorizontalHeaderLabels(column_labels);

  _model->setRowCount(lines.count());
  QStringList lineTokens;
  for (int row = 0; row < lines.count(); row++)
  {
    DataLoadCSV::SplitLine(lines[row], _dataLoadCSV->GetDelimiter(), lineTokens);
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
}

int DataLoadCSVUI::launchDialog(QFile& file, std::vector<std::string>* column_names)
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

    _dataLoadCSV->SetDelimeter(DataLoadCSV::DetectDelimiter(first_line));

    // Update the UI combobox to match the detected delimiter
    constexpr std::array<char, 4> delimiters = { ',', ';', ' ', '\t' };
    for (int i = 0; i < 4; i++)
    {
      if (delimiters[i] == _dataLoadCSV->GetDelimiter())
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
  connect(_ui->comboBox, qOverload<int>(&QComboBox::currentIndexChanged), context, [&](int index) {
    const std::array<char, 4> delimiters = { ',', ';', ' ', '\t' };
    _dataLoadCSV->SetDelimeter(delimiters[std::clamp(index, 0, 3)]);
    _csvHighlighter.delimiter = _dataLoadCSV->GetDelimiter();
    _dataLoadCSV->parseHeader(file, *column_names);
  });

  // parse the header once and launch the dialog
  _dataLoadCSV->parseHeader(file, *column_names);
  this->parseHeader(file, *column_names);

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
    return DataLoadCSV::TIME_INDEX_NOT_DEFINED;
  }

  if (_ui->radioButtonIndex->isChecked())
  {
    return DataLoadCSV::TIME_INDEX_GENERATED;
  }

  QModelIndexList indexes = _ui->listWidgetSeries->selectionModel()->selectedRows();
  if (indexes.size() == 1)
  {
    int row = indexes.front().row();
    auto item = _ui->listWidgetSeries->item(row);
    settings.setValue("DataLoadCSV.timeIndex", item->text());
    return row;
  }

  return DataLoadCSV::TIME_INDEX_NOT_DEFINED;
}

void DataLoadCSVUI::ShowWarningMessage(const QString& title, const QString& message)
{
  QMessageBox::warning(this, title, message);
}