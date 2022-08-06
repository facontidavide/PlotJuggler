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

void DataLoadMCAP::appendRollPitchYaw(const RosMsgParser::FlatMessage &flat,
                                      double timestamp,
                                      PlotDataMapRef &plot_data)
{
  static RosMsgParser::ROSType quaternion_type("geometry_msgs/Quaternion");
  const double RAD_TO_DEG = 180.0 / M_PI;

  double qx = 0, qy = 0, qz = 0, qw = 0;
  std::string prefix;
  bool quaternion_found = false;

  for(size_t i=0; i<flat.value.size(); i++ )
  {
    const auto& [key, val] = flat.value[i];
    if( key.fields.size() > 2 )
    {
      size_t last = key.fields.size() - 1;
      if(key.fields[last-1]->type() == quaternion_type)
      {
        if(key.fields[last]->type().typeID() == RosMsgParser::FLOAT64 &&
            key.fields[last]->name() == "x")
        {
          qx = val.convert<double>();
          qy = flat.value[i+1].second.convert<double>();
          qz = flat.value[i+2].second.convert<double>();
          qw = flat.value[i+3].second.convert<double>();
          quaternion_found = true;

          prefix = key.toStdString();
          prefix.pop_back();
          break;
        }
      }
    }
  }
  if( quaternion_found )
  {
    double quat_norm2 = (qw * qw) + (qx * qx) + (qy * qy) + (qz * qz);
    if (std::abs(quat_norm2 - 1.0) > std::numeric_limits<double>::epsilon())
    {
      double mult = 1.0 / std::sqrt(quat_norm2);
      qx *= mult;
      qy *= mult;
      qz *= mult;
      qw *= mult;
    }

    double roll, pitch, yaw;
    // roll (x-axis rotation)
    double sinr_cosp = 2 * (qw * qx + qy * qz);
    double cosr_cosp = 1 - 2 * (qx * qx + qy * qy);
    roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = 2 * (qw * qy - qz * qx);
    if (std::abs(sinp) >= 1)
    {
      pitch = std::copysign(M_PI_2, sinp);  // use 90 degrees if out of range
    }
    else
    {
      pitch = std::asin(sinp);
    }

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (qw * qz + qx * qy);
    double cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
    yaw = std::atan2(siny_cosp, cosy_cosp);

    PlotData& roll_data = plot_data.getOrCreateNumeric(prefix + "roll");
    roll_data.pushBack( {timestamp, RAD_TO_DEG*roll } );

    PlotData& pitch_data = plot_data.getOrCreateNumeric(prefix + "pitch");
    pitch_data.pushBack( {timestamp, RAD_TO_DEG*pitch } );

    PlotData& yaw_data = plot_data.getOrCreateNumeric(prefix + "yaw");
    yaw_data.pushBack( {timestamp, RAD_TO_DEG*yaw } );
  }
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

    std::cout  << definition << std::endl;

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

    appendRollPitchYaw(flat_output, timestamp_sec, plot_data);

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

