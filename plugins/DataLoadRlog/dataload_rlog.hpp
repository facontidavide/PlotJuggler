#pragma once

#include <QProgressDialog>
#include <QComboBox>
#include <QInputDialog>
#include <QDir>

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <bzlib.h>
#include <PlotJuggler/dataloader_base.h>
#include <capnp/serialize-packed.h>
#include <capnp/schema-parser.h>
#include <rlog_parser.hpp>

using namespace PJ;

class DataLoadRlog : public DataLoader 
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadRlog();
  virtual ~DataLoadRlog();
  virtual bool readDataFromFile(FileLoadInfo* fileload_info, PlotDataMapRef& plot_data);
  virtual const std::vector<const char*>& compatibleFileExtensions() const override;
  virtual const char* name() const override { return "DataLoad Rlog"; }
  virtual bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;
  virtual bool xmlLoadState(const QDomElement& parent_element) override;

  std::string SelectDBCDialog();

private:
  std::vector<const char*> _extensions;
  std::string _default_time_axis;
  bool can_dialog_tried = false;
};
