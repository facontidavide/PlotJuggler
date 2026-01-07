//
// Created by aGendy on 06/01/2026.
//

#ifndef PLOTJUGGLER_DATALOAD_CSV_UI_H
#define PLOTJUGGLER_DATALOAD_CSV_UI_H

#include "ui_dataload_csv.h"
#include "QCSVHighlighter"

class QStandardItemModel;
class DateTimeHelp;
class DataLoadCSV;

class DataLoadCSVUI : public QWidget
{
public:
  DataLoadCSVUI();

private slots:
  void ParseHeader(const QStringList& lines, const QString& preview_lines,
                   const std::vector<std::string>& column_names);

  void ShowWarningMessage(const QString& title, const QString& message);

  void LaunchDialog(QFile& file, std::vector<std::string>* column_names, int& result);

private:
  int launchDialog(QFile& file, std::vector<std::string>* column_names);

protected:
  ~DataLoadCSVUI() override;

private:
  std::unique_ptr<DataLoadCSV> _dataLoadCSV;

  QDialog* _dialog;
  Ui::DialogCSV* _ui;
  DateTimeHelp* _dateTime_dialog;
  QCSVHighlighter _csvHighlighter;
  QStandardItemModel* _model;
};

#endif  // PLOTJUGGLER_DATALOAD_CSV_UI_H
