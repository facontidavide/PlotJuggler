#pragma once

#include <QObject>
#include <QtPlugin>
#include <QWidget>

#include <string>
#include <unordered_set>

#include "PlotJuggler/dataloader_base.h"
#include "config_zcm.h"
#include "ui_dataload_zcm.h"

using namespace PJ;

class DataLoadZcm : public PJ::DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadZcm();

  virtual ~DataLoadZcm();

  const char* name() const override;

  const std::vector<const char*>& compatibleFileExtensions() const override;

  bool readDataFromFile(PJ::FileLoadInfo* fileload_info, PlotDataMapRef& destination) override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  QDialog* _dialog;
  ConfigZCM* _config_widget;
  Ui::DialogZcm* _ui;

  std::unordered_set<std::string> _all_channels;
  std::string _all_channels_filepath;

  std::unordered_set<std::string> _selected_channels;

  bool refreshChannels(const std::string& filepath);
  bool launchDialog(const std::string& filepath);
};
