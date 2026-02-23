#include "dataload_mcap.h"

#include "PlotJuggler/messageparser_base.h"

#include "mcap/reader.hpp"
#include "mcap/internal.hpp"
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
#include <QtConcurrent>

#include <set>

namespace
{

struct McapSummaryInfo
{
  std::unordered_map<mcap::SchemaId, mcap::SchemaPtr> schemas;
  std::unordered_map<mcap::ChannelId, mcap::ChannelPtr> channels;
  std::optional<mcap::Statistics> statistics;
  mcap::ByteOffset summaryStart = 0;
};

// Reads only Schema, Channel, and Statistics records from the MCAP summary
// by using SummaryOffset entries to seek directly to each group, skipping
// expensive MessageIndex and ChunkIndex data.
mcap::Status readSelectiveSummary(mcap::IReadable& reader, McapSummaryInfo& info)
{
  const uint64_t fileSize = reader.size();

  // 1. Read the Footer (last 37 bytes of the file)
  mcap::Footer footer;
  auto status =
      mcap::McapReader::ReadFooter(reader, fileSize - mcap::internal::FooterLength, &footer);
  if (!status.ok())
  {
    return status;
  }

  if (footer.summaryStart == 0)
  {
    return mcap::Status{ mcap::StatusCode::MissingStatistics, "no summary section" };
  }

  info.summaryStart = footer.summaryStart;

  const mcap::ByteOffset summaryOffsetStart = footer.summaryOffsetStart != 0 ?
                                                  footer.summaryOffsetStart :
                                                  fileSize - mcap::internal::FooterLength;

  if (summaryOffsetStart <= footer.summaryStart)
  {
    return mcap::Status{ mcap::StatusCode::InvalidFooter, "no SummaryOffset section available" };
  }

  // 2. Read the SummaryOffset section to find group byte ranges
  struct GroupRange
  {
    mcap::ByteOffset start = 0;
    mcap::ByteOffset end = 0;
  };
  GroupRange schemaRange, channelRange, statsRange;
  bool foundAny = false;

  mcap::RecordReader offsetReader(reader, summaryOffsetStart,
                                  fileSize - mcap::internal::FooterLength);
  while (auto record = offsetReader.next())
  {
    if (record->opcode != mcap::OpCode::SummaryOffset)
    {
      continue;
    }
    mcap::SummaryOffset so;
    if (!mcap::McapReader::ParseSummaryOffset(*record, &so).ok())
    {
      continue;
    }
    if (so.groupOpCode == mcap::OpCode::Schema)
    {
      schemaRange = { so.groupStart, so.groupStart + so.groupLength };
      foundAny = true;
    }
    else if (so.groupOpCode == mcap::OpCode::Channel)
    {
      channelRange = { so.groupStart, so.groupStart + so.groupLength };
      foundAny = true;
    }
    else if (so.groupOpCode == mcap::OpCode::Statistics)
    {
      statsRange = { so.groupStart, so.groupStart + so.groupLength };
      foundAny = true;
    }
  }

  if (!foundAny)
  {
    return mcap::Status{ mcap::StatusCode::MissingStatistics,
                         "no relevant SummaryOffset records found" };
  }

  // 3. Read each targeted group
  if (schemaRange.start != 0)
  {
    mcap::RecordReader rdr(reader, schemaRange.start, schemaRange.end);
    while (auto record = rdr.next())
    {
      if (record->opcode != mcap::OpCode::Schema)
      {
        continue;
      }
      auto ptr = std::make_shared<mcap::Schema>();
      if (mcap::McapReader::ParseSchema(*record, ptr.get()).ok())
      {
        info.schemas.try_emplace(ptr->id, ptr);
      }
    }
  }

  if (channelRange.start != 0)
  {
    mcap::RecordReader rdr(reader, channelRange.start, channelRange.end);
    while (auto record = rdr.next())
    {
      if (record->opcode != mcap::OpCode::Channel)
      {
        continue;
      }
      auto ptr = std::make_shared<mcap::Channel>();
      if (mcap::McapReader::ParseChannel(*record, ptr.get()).ok())
      {
        info.channels.try_emplace(ptr->id, ptr);
      }
    }
  }

  if (statsRange.start != 0)
  {
    mcap::RecordReader rdr(reader, statsRange.start, statsRange.end);
    while (auto record = rdr.next())
    {
      if (record->opcode != mcap::OpCode::Statistics)
      {
        continue;
      }
      mcap::Statistics stats;
      if (mcap::McapReader::ParseStatistics(*record, &stats).ok())
      {
        info.statistics = stats;
        break;  // only one Statistics record expected
      }
    }
  }

  if (!info.statistics)
  {
    return mcap::Status{ mcap::StatusCode::MissingStatistics,
                         "Statistics record not found in summary" };
  }

  return mcap::StatusCode::Success;
}

}  // anonymous namespace

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
  static std::vector<const char*> ext = { "mcap" };
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

  // --- Read summary information (schemas, channels, statistics) ---
  // Try selective read first (reads only Schema/Channel/Statistics via SummaryOffset,
  // skipping expensive MessageIndex and ChunkIndex data).
  // Falls back to full readSummary() for files without a SummaryOffset section.
  McapSummaryInfo summaryInfo;
  bool usedSelectiveSummary = false;
  status = readSelectiveSummary(*reader.dataSource(), summaryInfo);
  if (status.ok())
  {
    usedSelectiveSummary = true;
  }
  else
  {
    status = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
    if (!status.ok())
    {
      QMessageBox::warning(nullptr, "Can't open summary of the file",
                           tr("Code: %0\n Message: %1")
                               .arg(int(status.code))
                               .arg(QString::fromStdString(status.message)));
      return false;
    }
    for (const auto& [id, ptr] : reader.schemas())
    {
      summaryInfo.schemas.insert({ id, ptr });
    }
    for (const auto& [id, ptr] : reader.channels())
    {
      summaryInfo.channels.insert({ id, ptr });
    }
    summaryInfo.statistics = reader.statistics();
  }

  plot_data.addUserDefined("plotjuggler::mcap::file_path")
      ->second.pushBack({ 0, std::any(info->filename.toStdString()) });

  const auto& statistics = summaryInfo.statistics;

  std::unordered_map<int, mcap::SchemaPtr> mcap_schemas;         // schema_id
  std::unordered_map<int, mcap::ChannelPtr> channels;            // channel_id
  std::unordered_map<int, MessageParserPtr> parsers_by_channel;  // channel_id

  int total_dt_schemas = 0;

  std::unordered_set<mcap::ChannelId> channels_containing_datatamer_schema;
  std::unordered_set<mcap::ChannelId> channels_containing_datatamer_data;

  for (const auto& [schema_id, schema_ptr] : summaryInfo.schemas)
  {
    mcap_schemas.insert({ schema_id, schema_ptr });
  }

  if (!info->plugin_config.hasChildNodes())
  {
    _dialog_parameters = std::nullopt;
  }

  for (const auto& [channel_id, channel_ptr] : summaryInfo.channels)
  {
    channels.insert({ channel_id, channel_ptr });
  }

  // don't show the dialog if we already loaded the parameters with xmlLoadState
  if (!_dialog_parameters)
  {
    std::unordered_map<uint16_t, uint64_t> msg_count;
    if (statistics)
    {
      msg_count = statistics->channelMessageCounts;
    }
    DialogMCAP dialog(channels, mcap_schemas, msg_count, _dialog_parameters);
    auto ret = dialog.exec();
    if (ret != QDialog::Accepted)
    {
      return false;
    }
    _dialog_parameters = dialog.getParams();
  }

  std::set<QString> notified_encoding_problem;

  QElapsedTimer timer;
  timer.start();

  struct FailedParserInfo
  {
    std::set<std::string> topics;
    std::string error_message;
  };

  std::map<std::string, FailedParserInfo> parsers_blacklist;

  for (const auto& [channel_id, channel_ptr] : channels)
  {
    const auto& topic_name = channel_ptr->topic;
    const QString topic_name_qt = QString::fromStdString(topic_name);
    // skip topics that haven't been selected
    if (!_dialog_parameters->selected_topics.contains(topic_name_qt))
    {
      continue;
    }
    const auto& schema = mcap_schemas.at(channel_ptr->schemaId);

    // check if this schema is in the blacklist
    auto blacklist_it = parsers_blacklist.find(schema->name);
    if (blacklist_it != parsers_blacklist.end())
    {
      blacklist_it->second.topics.insert(channel_ptr->topic);
      continue;
    }

    const std::string definition(reinterpret_cast<const char*>(schema->data.data()),
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

    try
    {
      auto& parser_factory = it->second;
      auto parser = parser_factory->createParser(topic_name, schema->name, definition, plot_data);

      parsers_by_channel.insert({ channel_ptr->id, parser });
    }
    catch (std::exception& e)
    {
      FailedParserInfo failed_parser_info;
      failed_parser_info.error_message = e.what();
      failed_parser_info.topics.insert(channel_ptr->topic);
      parsers_blacklist.insert({ schema->name, failed_parser_info });
    }
  };

  // If any parser failed, show a message box with the error
  if (!parsers_blacklist.empty())
  {
    QString error_message;
    for (const auto& [schema_name, failed_parser_info] : parsers_blacklist)
    {
      error_message += QString("Schema: %1\n").arg(QString::fromStdString(schema_name));
      error_message +=
          QString("Error: %1\n").arg(QString::fromStdString(failed_parser_info.error_message));
      error_message += QString("Topics affected: \n");
      for (const auto& topic : failed_parser_info.topics)
      {
        error_message += QString(" - %1\n").arg(QString::fromStdString(topic));
      }
      error_message += "------------------\n";
    }
    QMessageBox::warning(nullptr, "Parser Error", error_message);
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
        total_msgs += statistics->channelMessageCounts.at(channel_id);
      }
    }
  }

  //-------------------------------------------
  //---------------- Parse messages -----------

  auto onProblem = [](const mcap::Status& problem) {
    qDebug() << QString::fromStdString(problem.message);
  };

  // When selective summary was used, readSummary() was not called, so
  // reader.dataEnd_ still includes the summary section. Construct
  // LinearMessageView with explicit byte range to avoid reading expensive
  // summary records during iteration.
  auto createMessageView = [&]() -> mcap::LinearMessageView {
    if (usedSelectiveSummary)
    {
      auto [dataStart, dataEndUnused] = reader.byteRange(0);
      return mcap::LinearMessageView(reader, dataStart, summaryInfo.summaryStart, 0, mcap::MaxTime,
                                     onProblem);
    }
    return reader.readMessages(onProblem);
  };

  auto messages = createMessageView();

  QProgressDialog progress_dialog("Loading... please wait", "Cancel", 0, 0, nullptr);
  progress_dialog.setWindowTitle("Loading the MCAP file");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, std::max<size_t>(total_msgs, 1) - 1);
  progress_dialog.show();
  progress_dialog.setValue(0);

  size_t msg_count = 0;

  auto new_progress_update = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

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

    if (msg_count++ % 100 == 0 && std::chrono::steady_clock::now() > new_progress_update)
    {
      new_progress_update += std::chrono::milliseconds(500);
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
