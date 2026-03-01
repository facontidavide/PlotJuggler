#pragma once

#include <QObject>
#include <QtPlugin>
#include <QWidget>
#include "PlotJuggler/dataloader_base.h"

using namespace PJ;

class DataLoadULog2 : public PJ::DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadULog2();

  const std::vector<const char*>& compatibleFileExtensions() const override;

  bool readDataFromFile(PJ::FileLoadInfo* fileload_info, PlotDataMapRef& destination) override;

  ~DataLoadULog2() override;

  const char* name() const override
  {
    return "DataLoad ULog2";
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  QWidget* _main_win;
};
