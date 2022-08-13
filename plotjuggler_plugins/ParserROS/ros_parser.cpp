#include "ros_parser.h"
#include "PlotJuggler/fmt/format.h"

using namespace PJ;
using namespace RosMsgParser;
static ROSType quaternion_type( Msg::Quaternion::id() );

ParserROS::ParserROS(const std::string& topic_name,
                     const std::string& type_name,
                     const std::string& schema,
                     RosMsgParser::Deserializer* deserializer,
                     PlotDataMapRef& data)
  : MessageParser(topic_name, data)
  ,_parser(topic_name, type_name, schema)
  ,_deserializer(deserializer)
  ,_topic(topic_name)
{
  auto policy = clampLargeArray() ? Parser::KEEP_LARGE_ARRAYS :
                                    Parser::DISCARD_LARGE_ARRAYS;

  _parser.setMaxArrayPolicy( policy, maxArraySize() );


  _is_diangostic_msg = ( Msg::DiagnosticStatus::id() == type_name);
  _is_jointstate_msg = ( Msg::JointStates::id() == type_name);
  _is_tf2_msg = ( Msg::TFMessage::id() == type_name);

  //---------- detect quaternion in schema --------
  auto hasQuaternion = [this](const auto& node) {
    if(node->value()->type() == quaternion_type)
    {
      _contains_quaternion = true;
    }
  };
  const auto& tree = _parser.getSchema()->field_tree;
  tree.visit(hasQuaternion, tree.croot());
}

bool ParserROS::parseMessage(const PJ::MessageRef serialized_msg, double &timestamp)
{
  if( _is_diangostic_msg )
  {
    parseDiagnosticMsg(serialized_msg, timestamp);
    return true;
  }

  _parser.deserialize(serialized_msg, &_flat_msg, _deserializer.get());

  std::string series_name;

  for(const auto& [key, str]: _flat_msg.name)
  {
    key.toStr(series_name);
    StringSeries& data = getStringSeries(series_name);
    data.pushBack( {timestamp, str } );
  }

  for(const auto& [key, value]: _flat_msg.value)
  {
    key.toStr(series_name);
    PlotData& data = getSeries(series_name);
    data.pushBack( {timestamp, value.convert<double>() } );
  }

  if( _contains_quaternion )
  {
    appendRollPitchYaw(timestamp);
  }
  return true;
}

void ParserROS::setLargeArraysPolicy(bool clamp, unsigned max_size)
{
  auto policy = clamp ? RosMsgParser::Parser::KEEP_LARGE_ARRAYS :
                        RosMsgParser::Parser::DISCARD_LARGE_ARRAYS;

  _parser.setMaxArrayPolicy( policy, max_size );  
  MessageParser::setLargeArraysPolicy(clamp, max_size);
}

void ParserROS::appendRollPitchYaw(double stamp)
{
  const double RAD_TO_DEG = 180.0 / M_PI;

  for(size_t i=0; i< _flat_msg.value.size(); i++ )
  {
    const auto& [key, val] = _flat_msg.value[i];

    if( key.fields.size() < 2 || (i+3) >= _flat_msg.value.size() )
    {
      continue;
    }
    size_t last = key.fields.size() - 1;

    if( key.fields[last-1]->type() == quaternion_type &&
        key.fields[last]->type().typeID() == RosMsgParser::FLOAT64 &&
        key.fields[last]->name() == "x")
    {
      Msg::Quaternion quat;

      quat.x = val.convert<double>();
      quat.y = _flat_msg.value[i+1].second.convert<double>();
      quat.z = _flat_msg.value[i+2].second.convert<double>();
      quat.w = _flat_msg.value[i+3].second.convert<double>();

      std::string prefix = key.toStdString();
      prefix.pop_back();

      auto rpy = Msg::QuaternionToRPY( quat );

      getSeries(prefix + "roll_deg").pushBack( {stamp, RAD_TO_DEG * rpy.roll } );
      getSeries(prefix + "pitch_deg").pushBack( {stamp, RAD_TO_DEG * rpy.pitch } );
      getSeries(prefix + "yaw_deg").pushBack( {stamp, RAD_TO_DEG * rpy.yaw } );

      break;
    }
  }
}

void ParserROS::parseDiagnosticMsg(const PJ::MessageRef serialized_msg, double &timestamp)
{
  using namespace RosMsgParser;

  thread_local Msg::DiagnosticArray msg;
  _deserializer->init( Span<const uint8_t>(serialized_msg.data(), serialized_msg.size()) );

  // only ROS1 as the files header.seq
  if( dynamic_cast<ROS_Deserializer*>(_deserializer.get()) )
  {
    msg.header.seq = _deserializer->deserializeUInt32();
  }

  msg.header.stamp.sec = _deserializer->deserializeUInt32();
  msg.header.stamp.nanosec = _deserializer->deserializeUInt32();
  _deserializer->deserializeString(msg.header.frame_id);

  size_t status_count = _deserializer->deserializeUInt32();
  msg.status.resize( status_count );
  for(size_t st = 0; st < status_count; st++)
  {
    auto& status = msg.status[st];
    status.level = _deserializer->deserialize(BYTE).convert<uint8_t>();
    _deserializer->deserializeString(status.name);
    _deserializer->deserializeString(status.message);
    _deserializer->deserializeString(status.hardware_id);

    status.key_value.clear();

    size_t key_value_count = _deserializer->deserializeUInt32();
    std::string key;
    std::string value_str;
    for(size_t kv = 0; kv < key_value_count; kv++)
    {
      _deserializer->deserializeString(key);
      _deserializer->deserializeString(value_str);
      status.key_value.push_back( {key, value_str} );
    }
  }

  //------ Now create the series --------
  double ts = timestamp;
  getSeries(fmt::format("{}/header/seq", _topic)).pushBack( {ts, double(msg.header.seq)} );
  getSeries(fmt::format("{}/header/stamp", _topic)).pushBack( {ts, msg.header.stamp.toSec()} );
  getStringSeries(fmt::format("{}/header/frame_id", _topic)).pushBack( {ts, msg.header.frame_id} );

  std::string series_name;

  for (const auto& status : msg.status)
  {
    for (const auto& kv : status.key_value)
    {
      if (status.hardware_id.empty())
      {
        series_name = fmt::format("{}/{}/{}",
                                  _topic, status.name, kv.first);
      }
      else {
        series_name = fmt::format("{}/{}/{}/{}",
                                  _topic, status.hardware_id, status.name, kv.first);
      }

      bool ok;
      double value = QString::fromStdString(kv.second).toDouble(&ok);

      if (ok)
      {
        getSeries(series_name).pushBack({ timestamp, value });
      }
      else {
        getStringSeries(series_name).pushBack({ timestamp, kv.second });
      }
    }
  }
}
