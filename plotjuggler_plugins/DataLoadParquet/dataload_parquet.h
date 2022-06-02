#pragma once

#include <QObject>
#include <QtPlugin>
#include "PlotJuggler/dataloader_base.h"
#include "ui_dataload_parquet.h"

#define QT_NO_KEYWORDS
#undef signals
#include <parquet/stream_reader.h>

using namespace PJ;

class DataLoadParquet : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadParquet();
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;

  virtual bool readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                PlotDataMapRef& destination) override;

  ~DataLoadParquet() override;

  virtual const char* name() const override
  {
    return "DataLoad Parquet";
  }

  QString selectedSeries() const;

  virtual bool xmlSaveState(QDomDocument& doc,
                            QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;


private:

  Ui::DialogParquet* ui = nullptr;

  std::vector<const char*> _extensions;

  QString _default_time_axis;

  std::unique_ptr<parquet::ParquetFileReader> parquet_reader_;

  void setupDialog(QDialog* dialog);
};
