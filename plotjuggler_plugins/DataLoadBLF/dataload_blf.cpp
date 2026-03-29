#include "dataload_blf.h"

#include <QDebug>
#include <QMessageBox>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "blf_decoder.h"
#include "blf_reader.h"
#include "dbcppp_decoder.h"
#include "dialog_blf.h"

namespace PJ::BLF
{
namespace
{
constexpr const char* kHasValidAbsoluteTimeProperty = "pj_blf_has_valid_absolute_time";
constexpr const char* kFirstAbsoluteTimeMsecProperty = "pj_blf_first_absolute_time_msec";

class PlotDataSeriesWriter : public ISeriesWriter
{
public:
  explicit PlotDataSeriesWriter(PJ::PlotDataMapRef& destination) : destination_(destination)
  {
  }

  void WriteSample(const std::string& series_name, double timestamp, double value) override
  {
    auto& series = destination_.getOrCreateNumeric(series_name, nullptr);
    series.pushBack({timestamp, value});
  }

private:
  PJ::PlotDataMapRef& destination_;
};

}  // namespace

const std::vector<const char*>& DataLoadBLF::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = {"blf"};
  return extensions;
}

bool DataLoadBLF::readDataFromFile(PJ::FileLoadInfo* fileload_info, PJ::PlotDataMapRef& destination)
{
  if (!fileload_info)
  {
    return false;
  }

  last_metadata_ = {};
  setProperty(kHasValidAbsoluteTimeProperty, false);
  setProperty(kFirstAbsoluteTimeMsecProperty, QVariant());

  BlfPluginConfig runtime_config = config_;
  if (fileload_info->plugin_config.hasChildNodes())
  {
    BlfPluginConfig loaded_config;
    std::string cfg_error;
    const QDomElement root = fileload_info->plugin_config.documentElement();
    if (!root.isNull() && LoadConfigFromXml(root, loaded_config, &cfg_error))
    {
      runtime_config = std::move(loaded_config);
    }
  }

  DialogBLF dialog;
  dialog.SetConfig(runtime_config);
  if (dialog.exec() != QDialog::Accepted)
  {
    return false;
  }
  runtime_config = dialog.GetConfig();
  config_ = runtime_config;
  last_metadata_.dbc_files = runtime_config.dbc_files;

  std::vector<std::string> decoder_errors;
  DbcManager dbc_manager([&decoder_errors](const std::string& dbc_file) {
    std::string decoder_error;
    auto decoder = std::make_unique<DbcpppDecoder>(dbc_file, &decoder_error);
    if (!decoder->IsValid())
    {
      if (!decoder_error.empty())
      {
        decoder_errors.push_back(decoder_error);
      }
      return std::unique_ptr<IDbcDecoder>();
    }
    return std::unique_ptr<IDbcDecoder>(std::move(decoder));
  });

  BlfPluginConfig run_config = runtime_config;
  DbcManager* manager_ptr = nullptr;

  if (run_config.emit_decoded)
  {
    std::vector<ChannelDbcBinding> bindings;
    bindings.reserve(run_config.channel_to_dbc.size());
    for (const auto& [channel, dbc_file] : run_config.channel_to_dbc)
    {
      bindings.push_back(ChannelDbcBinding{channel, dbc_file});
    }

    if (!bindings.empty())
    {
      std::string load_error;
      if (!dbc_manager.LoadBindings(bindings, &load_error))
      {
        if (!decoder_errors.empty())
        {
          load_error += "\n";
          load_error += decoder_errors.front();
        }
        if (!run_config.emit_raw)
        {
          QMessageBox::warning(nullptr, tr("BLF Load Failed"),
                               tr("DBC loading failed and raw output is disabled.\n%1")
                                   .arg(QString::fromStdString(load_error)));
          return false;
        }

        run_config.emit_decoded = false;
        QMessageBox::warning(nullptr, tr("DBC Decode Disabled"),
                             tr("DBC mapping is invalid, continue with raw series only.\n%1")
                                 .arg(QString::fromStdString(load_error)));
      }
      else
      {
        manager_ptr = &dbc_manager;
      }
    }
    else
    {
      if (!run_config.emit_raw)
      {
        QMessageBox::warning(nullptr, tr("BLF Load Failed"),
                             tr("Decoded output is enabled, but no channel-to-DBC mappings are configured."));
        return false;
      }

      run_config.emit_decoded = false;
      QMessageBox::warning(nullptr, tr("DBC Decode Disabled"),
                           tr("No channel-to-DBC mapping configured, continue with raw series only."));
    }
  }

  PlotDataSeriesWriter writer(destination);
  BlfDecoderPipeline pipeline(run_config, manager_ptr, &writer);

  uint64_t frames_count = 0;
  BlfReader reader;
  QString read_error;
  const bool read_ok =
      reader.ReadFrames(fileload_info->filename,
                        [&](const NormalizedCanFrame& frame) {
                          ++frames_count;
                          pipeline.ProcessFrame(frame);
                        },
                        read_error,
                        &last_metadata_);
  if (!read_ok)
  {
    QMessageBox::warning(nullptr, tr("BLF Read Failed"),
                         tr("Failed to parse BLF file:\n%1").arg(read_error));
    return false;
  }

  if (frames_count == 0U)
  {
    QMessageBox::warning(nullptr, tr("BLF Read Failed"),
                         tr("No CAN/CAN FD frames were found in the BLF file."));
    return false;
  }

  const BlfDecodeStats stats = pipeline.stats();
  qInfo().nospace() << "DataLoadBLF summary: frames=" << stats.frames_processed
                    << ", raw_samples=" << stats.raw_samples_written
                    << ", decoded_samples=" << stats.decoded_samples_written
                    << ", decode_errors=" << stats.decode_errors;

  setProperty(kHasValidAbsoluteTimeProperty, last_metadata_.has_valid_absolute_time);
  setProperty(kFirstAbsoluteTimeMsecProperty,
              QVariant::fromValue<qlonglong>(last_metadata_.first_absolute_time_msec));

  return true;
}

bool DataLoadBLF::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  SaveConfigToXml(config_, doc, parent_element);
  return true;
}

bool DataLoadBLF::xmlLoadState(const QDomElement& parent_element)
{
  BlfPluginConfig loaded;
  std::string error;
  if (!LoadConfigFromXml(parent_element, loaded, &error))
  {
    return false;
  }
  config_ = std::move(loaded);
  return true;
}

}  // namespace PJ::BLF
