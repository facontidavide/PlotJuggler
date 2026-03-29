#pragma once

#include <QObject>
#include <QtPlugin>

#include "PlotJuggler/dataloader_base.h"
#include "blf_reader.h"
#include "blf_config.h"

namespace PJ::BLF
{

class DataLoadBLF : public PJ::DataLoader
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataLoader")
  Q_INTERFACES(PJ::DataLoader)

public:
  DataLoadBLF() = default;
  ~DataLoadBLF() override = default;

  const std::vector<const char*>& compatibleFileExtensions() const override;
  bool readDataFromFile(PJ::FileLoadInfo* fileload_info, PJ::PlotDataMapRef& destination) override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;
  bool xmlLoadState(const QDomElement& parent_element) override;

  const char* name() const override
  {
    return "DataLoad BLF";
  }

  const BlfLoadMetadata& lastLoadMetadata() const
  {
    return last_metadata_;
  }

private:
  BlfPluginConfig config_;
  BlfLoadMetadata last_metadata_;
};

}  // namespace PJ::BLF
