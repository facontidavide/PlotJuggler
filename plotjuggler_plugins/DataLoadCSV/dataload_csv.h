#pragma once

#include <QObject>
#include <QtPlugin>
#include <QStandardItemModel>
#include "PlotJuggler/dataloader_base.h"
#include "ui_dataload_csv.h"
#include "QCSVHighlighter"

using namespace PJ;

class DateTimeHelp;

class DataLoadCSV : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadCSV();
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;

  virtual bool readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                PlotDataMapRef& destination) override;

  virtual ~DataLoadCSV();

  virtual const char* name() const override
  {
    return "DataLoad CSV";
  }

  virtual bool xmlSaveState(QDomDocument& doc,
                            QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;

protected:
  void parseHeader(QFile& file, std::vector<std::string>& ordered_names);

  int launchDialog(QFile& file, std::vector<std::string>* ordered_names);

private:
  std::vector<const char*> _extensions;

  std::string _default_time_axis;

  QChar _delimiter;

  QCSVHighlighter _csvHighlighter;

  FileLoadInfo* _fileInfo;

  QDialog* _dialog;
  Ui::DialogCSV* _ui;
  DateTimeHelp *_dateTime_dialog;

  QStandardItemModel *_model;

  bool multiple_columns_warning_ = true;

};
