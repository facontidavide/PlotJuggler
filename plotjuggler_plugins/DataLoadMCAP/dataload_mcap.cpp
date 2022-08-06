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
#include "rosx_introspection/ros_parser.hpp"

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
  // open file
  std::ifstream input(info->filename.toStdString(), std::ios::binary);
  mcap::FileStreamReader data_source(input);

  // read metainfo
  mcap::TypedRecordReader type_reader(data_source, 8);

  std::unordered_map<int, mcap::SchemaPtr> schemas;
  std::unordered_map<int, mcap::ChannelPtr> channels;
  std::unordered_map<std::string, std::shared_ptr<RosMsgParser::Deserializer>> deserializers;
  std::unordered_map<int, std::shared_ptr<RosMsgParser::Parser>> ros_parsers;
  std::unordered_map<int, RosMsgParser::FlatMessage> flat_outputs;

  deserializers["ros1"] = std::make_shared<RosMsgParser::ROS_Deserializer>();
  deserializers["cdr"] = std::make_shared<RosMsgParser::FastCDR_Deserializer>();
  //TODO protobuf

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
    const auto& topic = recordPtr->topic;
    std::string definition(reinterpret_cast<const char*>(schema->data.data()),
                           schema->data.size());

    std::string type_name = QString::fromStdString(schema->name).replace("/msg/", "/").toStdString();

    auto ros_type = RosMsgParser::ROSType(type_name);
    auto parser = std::make_shared<RosMsgParser::Parser>(topic, ros_type, definition);

    ros_parsers.insert( {recordPtr->id, parser} );
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

  auto array_policy = dialog_params.clamp_large_arrays ?
    RosMsgParser::Parser::KEEP_LARGE_ARRAYS :
    RosMsgParser::Parser::DISCARD_LARGE_ARRAYS;

  std::unordered_set<int> enabled_channels;

  for (const auto& [channel_id, parser] : ros_parsers)
  {
    parser->setMaxArrayPolicy(array_policy, dialog_params.max_array_size);
    QString topic_name = QString::fromStdString(channels[channel_id]->topic);
    if( dialog_params.selected_topics.contains(topic_name) )
    {
      enabled_channels.insert(channel_id);
    }
  }

  //-------------------------------------------
  //---------------- Parse messages -----------------
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

    const auto& encoding = msg_view.channel->messageEncoding;
    auto des_it = deserializers.find(encoding);

    if( des_it == deserializers.end() )
    {
      qDebug() << "Skipping encoding: " << QString::fromStdString(encoding);
      continue;
    }
    auto parser_it = ros_parsers.find(msg_view.channel->id);
    if( parser_it == ros_parsers.end() )
    {
      qDebug() << "Skipping channeld id: " << msg_view.channel->id;
      continue;
    }
    auto parser = parser_it->second;
    const uint8_t* buffer_ptr = reinterpret_cast<const uint8_t*>(msg_view.message.data);
    RosMsgParser::Span<const uint8_t> buffer(buffer_ptr, msg_view.message.dataSize);

    auto& flat_output = flat_outputs[msg_view.channel->id];
    parser->deserialize(buffer, &flat_output, des_it->second.get());

    std::string series_name;
    for(const auto& [key, value]: flat_output.value)
    {
      key.toStr(series_name);
      PlotData& data = plot_data.getOrCreateNumeric(series_name);
      data.pushBack( {timestamp_sec, value.convert<double>() } );
    }
    for(const auto& [key, str]: flat_output.name)
    {
      key.toStr(series_name);
      StringSeries& data = plot_data.getOrCreateStringSeries(series_name);
      data.pushBack( {timestamp_sec, str } );
    }
  }

  msg_reader.close();

  return true;
}

