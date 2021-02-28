#pragma once

#include <QObject>
#include <QtPlugin>
#include "PlotJuggler/dataloader_base.h"
#include <dbcppp/Network.h>

using namespace PJ;

class DataLoadCAN : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadCAN();
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;
  virtual QSize inspectFile(QFile* file);
  virtual bool readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                PlotDataMapRef& destination) override;
  virtual bool loadCANDatabase(QString dbc_filename);
  virtual ~DataLoadCAN();

  virtual const char* name() const override
  {
    return "DataLoad CAN";
  }

  virtual bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;

protected:
  QSize parseHeader(QFile* file, std::vector<std::string>& ordered_names);

private:
  std::vector<const char*> _extensions;

  std::string _default_time_axis;
  std::unique_ptr<dbcppp::Network> can_network_;
};


