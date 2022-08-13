#include "dataload_mcap.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QPushButton>
#include "QSyntaxStyle"

#include "mcap/reader.hpp"
#include "dialog_mcap.h"
#include "PlotJuggler/fmt/format.h"

#include <QStandardItemModel>

DataLoadMCAP::DataLoadMCAP()
{

}

DataLoadMCAP::~DataLoadMCAP()
{

}

const std::vector<const char*>& DataLoadMCAP::compatibleFileExtensions() const
{
  static std::vector<const char*> ext = {"mcap", "MCAP"};
  return ext;
}


bool DataLoadMCAP::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  if( !parserFactories() )
  {
    throw std::runtime_error("No parsing available");
  }
  // open file
  std::ifstream input(info->filename.toStdString(), std::ios::binary);
  mcap::FileStreamReader data_source(input);

  // read metainfo
  mcap::TypedRecordReader type_reader(data_source, 8);

  std::unordered_map<int, mcap::SchemaPtr> schemas; // schema_id
  std::unordered_map<int, mcap::ChannelPtr> channels; // channel_id
  std::unordered_map<int, MessageParserPtr> parsers_by_channel; // channel_id

  type_reader.onSchema = [&](const mcap::SchemaPtr recordPtr,
                             mcap::ByteOffset, std::optional<mcap::ByteOffset>)
  {
    schemas.insert( {recordPtr->id, recordPtr} );
  };

  type_reader.onChannel = [&](const mcap::ChannelPtr recordPtr,
                              mcap::ByteOffset, std::optional<mcap::ByteOffset>)
  {
    channels.insert( {recordPtr->id, recordPtr} );

    auto schema = schemas.at(recordPtr->schemaId);
    const auto& topic_name = recordPtr->topic;
    std::string definition(reinterpret_cast<const char*>(schema->data.data()),
                           schema->data.size());

    QString encoding = QString::fromStdString(recordPtr->messageEncoding);

    auto it = parserFactories()->find( encoding );

    if(it == parserFactories()->end() )
    {
      encoding = QString::fromStdString(schema->encoding);
      it = parserFactories()->find( encoding );
    }

    if(it == parserFactories()->end() )
    {
      throw std::runtime_error(
        fmt::format("No parsing available for encoding [{}] nor [{}]",
                    schema->encoding, recordPtr->messageEncoding) );
    }

    auto& parser_factory = it->second;
    auto parser = parser_factory->createParser(topic_name,
                                               schema->name,
                                               definition,
                                               plot_data);
    parsers_by_channel.insert( {recordPtr->id, parser} );
  };

  bool running = true;
  while (running)
  {
    running = type_reader.next();
    if (!type_reader.status().ok())
    {
      QMessageBox::warning(nullptr, tr("MCAP parsing"),
                           QString("Error reading the MCAP file:\n%1").arg(info->filename),
                           QMessageBox::Cancel);
      return false;
    }
  }

  DialogMCAP dialog(channels, schemas);
  auto ret = dialog.exec();
  if (ret != QDialog::Accepted)
  {
    return false;
  }

  auto dialog_params = dialog.getParams();

  std::unordered_set<int> enabled_channels;

  for (const auto& [channel_id, parser] : parsers_by_channel)
  {
    parser->setLargeArraysPolicy(dialog_params.clamp_large_arrays,
                                 dialog_params.max_array_size);

    QString topic_name = QString::fromStdString(channels[channel_id]->topic);
    if( dialog_params.selected_topics.contains(topic_name) )
    {
      enabled_channels.insert(channel_id);
    }
  }

  //-------------------------------------------
  //---------------- Parse messages -----------
  mcap::McapReader msg_reader;
  auto status = msg_reader.open(data_source);
  if (!status.ok())
  {
    auto msg = QString::fromStdString(status.message);
    QMessageBox::warning(nullptr, "MCAP parsing",
                         QString("Error reading the MCAP file: %1").arg(msg),
                         QMessageBox::Cancel);
    return false;
  }

  auto onProblem = [](const mcap::Status& problem) {
    qDebug() << QString::fromStdString(problem.message);
  };

  auto messages = msg_reader.readMessages(onProblem);

  for (const auto& msg_view : messages)
  {
    if( enabled_channels.count(msg_view.channel->id) == 0 )
    {
      continue;
    }

    // MCAP always represents publishTime in nanoseconds
    double timestamp_sec = double(msg_view.message.publishTime) * 1e-9;

    auto parser_it = parsers_by_channel.find(msg_view.channel->id);
    if( parser_it == parsers_by_channel.end() )
    {
      qDebug() << "Skipping channeld id: " << msg_view.channel->id;
      continue;
    }

    auto parser = parser_it->second;
    MessageRef msg(msg_view.message.data, msg_view.message.dataSize);
    parser->parseMessage(msg, timestamp_sec);
  }

  msg_reader.close();

  return true;
}

