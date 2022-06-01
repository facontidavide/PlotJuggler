#pragma once

#include <QObject>
#include <QtPlugin>
#include "PlotJuggler/dataloader_base.h"
#include "ui_dataload_parquet.h"

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

  virtual ~DataLoadParquet();

  virtual const char* name() const override
  {
    return "DataLoad Parquet";
  }

  virtual bool xmlSaveState(QDomDocument& doc,
                            QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;


private:
  std::vector<const char*> _extensions;

  std::string _default_time_axis;

  Ui::DialogParquet* _ui;

};
