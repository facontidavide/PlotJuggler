#include "dataload_mcap.h"

#include "PlotJuggler/messageparser_base.h"

#include "mcap/reader.hpp"
#include "dialog_mcap.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QPushButton>
#include <QElapsedTimer>
#include <QStandardItemModel>

#include <set>

DataLoadMCAP::DataLoadMCAP()
{
}

DataLoadMCAP::~DataLoadMCAP()
{
}

bool DataLoadMCAP::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  if (!_dialog_parameters)
  {
    return false;
  }
  QDomElement elem = doc.createElement("parameters");
  const auto& params = *_dialog_parameters;
  elem.setAttribute("use_timestamp", int(params.use_timestamp));
  elem.setAttribute("use_mcap_log_time", int(params.use_mcap_log_time));
  elem.setAttribute("clamp_large_arrays", int(params.clamp_large_arrays));
  elem.setAttribute("max_array_size", params.max_array_size);
  elem.setAttribute("selected_topics", params.selected_topics.join(';'));

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadMCAP::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("parameters");
  if (elem.isNull())
  {
    _dialog_parameters = std::nullopt;
    return false;
  }
  mcap::LoadParams params;
  params.use_timestamp = bool(elem.attribute("use_timestamp").toInt());
  params.use_mcap_log_time = bool(elem.attribute("use_mcap_log_time").toInt());
  params.clamp_large_arrays = bool(elem.attribute("clamp_large_arrays").toInt());
  params.max_array_size = elem.attribute("max_array_size").toInt();
  params.selected_topics = elem.attribute("selected_topics").split(';');
  _dialog_parameters = params;
  return true;
}

const std::vector<const char*>& DataLoadMCAP::compatibleFileExtensions() const
{
  static std::vector<const char*> ext = { "mcap", "MCAP" };
  return ext;
}

bool DataLoadMCAP::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  if (!parserFactories())
  {
    throw std::runtime_error("No parsing available");
  }

  // open file
  mcap::McapReader reader;
  auto status = reader.open(info->filename.toStdString());
  if (!status.ok())
  {
    QMessageBox::warning(nullptr, "Can't open file",
                         tr("Code: %0\n Message: %1")
                             .arg(int(status.code))
                             .arg(QString::fromStdString(status.message)));
    return false;
  }

  status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
  if (!status.ok())
  {
    QMessageBox::warning(nullptr, "Can't open summary of the file",
                         tr("Code: %0\n Message: %1")
                             .arg(int(status.code))
                             .arg(QString::fromStdString(status.message)));
    return false;
  }
  plot_data.addUserDefined("plotjuggler::mcap::file_path")
      ->second.pushBack({ 0, std::any(info->filename.toStdString()) });

  auto statistics = reader.statistics();

  std::unordered_map<int, mcap::SchemaPtr> mcap_schemas;         // schema_id
  std::unordered_map<int, mcap::ChannelPtr> channels;            // channel_id
  std::unordered_map<int, MessageParserPtr> parsers_by_channel;  // channel_id

  int total_dt_schemas = 0;

  std::unordered_set<mcap::ChannelId> channels_containing_datatamer_schema;
  std::unordered_set<mcap::ChannelId> channels_containing_datatamer_data;

  for (const auto& [schema_id, schema_ptr] : reader.schemas())
  {
    mcap_schemas.insert({ schema_id, schema_ptr });
  }

  std::set<QString> notified_encoding_problem;

  QElapsedTimer timer;
  timer.start();

  for (const auto& [channel_id, channel_ptr] : reader.channels())
  {
    channels.insert({ channel_id, channel_ptr });
    const auto& schema = mcap_schemas.at(channel_ptr->schemaId);
    const auto& topic_name = channel_ptr->topic;
    std::string definition(reinterpret_cast<const char*>(schema->data.data()),
                           schema->data.size());

    if (schema->name == "data_tamer_msgs/msg/Schemas")
    {
      channels_containing_datatamer_schema.insert(channel_id);
      total_dt_schemas += statistics->channelMessageCounts.at(channel_id);
    }
    if (schema->name == "data_tamer_msgs/msg/Snapshot")
    {
      channels_containing_datatamer_data.insert(channel_id);
    }

    QString channel_encoding = QString::fromStdString(channel_ptr->messageEncoding);
    QString schema_encoding = QString::fromStdString(schema->encoding);

    auto it = parserFactories()->find(channel_encoding);

    if (it == parserFactories()->end())
    {
      it = parserFactories()->find(schema_encoding);
    }

    if (it == parserFactories()->end())
    {
      // show message only once per encoding type
      if (notified_encoding_problem.count(schema_encoding) == 0)
      {
        notified_encoding_problem.insert(schema_encoding);
        auto msg = QString("No parser available for encoding [%0] nor [%1]")
                       .arg(channel_encoding)
                       .arg(schema_encoding);
        QMessageBox::warning(nullptr, "Encoding problem", msg);
      }
      continue;
    }

    auto& parser_factory = it->second;
    auto parser =
        parser_factory->createParser(topic_name, schema->name, definition, plot_data);
    parsers_by_channel.insert({ channel_ptr->id, parser });
  };

  if (!info->plugin_config.hasChildNodes())
  {
    _dialog_parameters = std::nullopt;
  }

  // don't show the dialog if we already loaded the parameters with xmlLoadState
  if (!_dialog_parameters)
  {
    DialogMCAP dialog(channels, mcap_schemas, _dialog_parameters);
    auto ret = dialog.exec();
    if (ret != QDialog::Accepted)
    {
      return false;
    }
    _dialog_parameters = dialog.getParams();
  }

  std::unordered_set<int> enabled_channels;
  size_t total_msgs = 0;

  for (const auto& [channel_id, parser] : parsers_by_channel)
  {
    parser->setLargeArraysPolicy(_dialog_parameters->clamp_large_arrays,
                                 _dialog_parameters->max_array_size);
    parser->enableEmbeddedTimestamp(_dialog_parameters->use_timestamp);

    QString topic_name = QString::fromStdString(channels[channel_id]->topic);
    if (_dialog_parameters->selected_topics.contains(topic_name))
    {
      enabled_channels.insert(channel_id);
      auto mcap_channel = channels[channel_id]->id;
      if (statistics->channelMessageCounts.count(mcap_channel) != 0)
      {
        total_msgs += statistics->channelMessageCounts[channels[channel_id]->id];
      }
    }
  }

  //-------------------------------------------
  //---------------- Parse messages -----------

  auto onProblem = [](const mcap::Status& problem) {
    qDebug() << QString::fromStdString(problem.message);
  };

  auto messages = reader.readMessages(onProblem);

  QProgressDialog progress_dialog("Loading... please wait", "Cancel", 0, 0, nullptr);
  progress_dialog.setWindowTitle("Loading the MCAP file");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, std::max<size_t>(total_msgs, 1) - 1);
  progress_dialog.show();
  progress_dialog.setValue(0);

  size_t msg_count = 0;

  for (const auto& msg_view : messages)
  {
    if (enabled_channels.count(msg_view.channel->id) == 0)
    {
      continue;
    }

    // MCAP always represents publishTime in nanoseconds
    double timestamp_sec = double(msg_view.message.publishTime) * 1e-9;
    if (_dialog_parameters->use_mcap_log_time)
    {
      timestamp_sec = double(msg_view.message.logTime) * 1e-9;
    }
    auto parser_it = parsers_by_channel.find(msg_view.channel->id);
    if (parser_it == parsers_by_channel.end())
    {
      qDebug() << "Skipping channeld id: " << msg_view.channel->id;
      continue;
    }

    auto parser = parser_it->second;
    MessageRef msg(msg_view.message.data, msg_view.message.dataSize);
    parser->parseMessage(msg, timestamp_sec);

    if (msg_count++ % 1000 == 0)
    {
      progress_dialog.setValue(msg_count);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        break;
      }
    }
  }

  reader.close();
  qDebug() << "Loaded file in " << timer.elapsed() << "milliseconds";
  return true;
}
