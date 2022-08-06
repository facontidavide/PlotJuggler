#pragma once

#include <QObject>
#include <QtPlugin>
#include <QStandardItemModel>
#include "PlotJuggler/dataloader_base.h"
#include "rosx_introspection/ros_parser.hpp"

using namespace PJ;

class DataLoadMCAP : public DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadMCAP();
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;

  virtual bool readDataFromFile(PJ::FileLoadInfo* fileload_info,
                                PlotDataMapRef& destination) override;

  virtual ~DataLoadMCAP() override;

  virtual const char* name() const override
  {
    return "DataLoad MCAP";
  }

private:

  void appendRollPitchYaw(const RosMsgParser::FlatMessage& flat, double timestamp,
                          PlotDataMapRef& plot_data);
};
