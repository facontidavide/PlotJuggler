#include "ros_parser.h"

using namespace PJ;

ParserROS::ParserROS(const std::string& topic_name,
                     const std::string &type_name,
                     const std::string& schema,
                     RosMsgParser::Deserializer* deserializer,
                     PlotDataMapRef& data)
  : MessageParser(topic_name, data)
  ,_parser(topic_name, type_name, schema)
  ,_deserializer(deserializer)
{
}

bool ParserROS::parseMessage(const PJ::MessageRef serialized_msg, double &timestamp)
{
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

  //  appendRollPitchYaw(flat_output, timestamp_sec, plot_data);
  return true;
}

//void ParserROS::appendRollPitchYaw(const RosMsgParser::FlatMessage &flat,
//                                      double timestamp,
//                                      PlotDataMapRef &plot_data)
//{
//  static RosMsgParser::ROSType quaternion_type("geometry_msgs/Quaternion");
//  const double RAD_TO_DEG = 180.0 / M_PI;

//  double qx = 0, qy = 0, qz = 0, qw = 0;
//  std::string prefix;
//  bool quaternion_found = false;

//  for(size_t i=0; i<flat.value.size(); i++ )
//  {
//    const auto& [key, val] = flat.value[i];
//    if( key.fields.size() > 2 )
//    {
//      size_t last = key.fields.size() - 1;
//      if(key.fields[last-1]->type() == quaternion_type)
//      {
//        if(key.fields[last]->type().typeID() == RosMsgParser::FLOAT64 &&
//            key.fields[last]->name() == "x")
//        {
//          qx = val.convert<double>();
//          qy = flat.value[i+1].second.convert<double>();
//          qz = flat.value[i+2].second.convert<double>();
//          qw = flat.value[i+3].second.convert<double>();
//          quaternion_found = true;

//          prefix = key.toStdString();
//          prefix.pop_back();
//          break;
//        }
//      }
//    }
//  }
//  if( quaternion_found )
//  {
//    double quat_norm2 = (qw * qw) + (qx * qx) + (qy * qy) + (qz * qz);
//    if (std::abs(quat_norm2 - 1.0) > std::numeric_limits<double>::epsilon())
//    {
//      double mult = 1.0 / std::sqrt(quat_norm2);
//      qx *= mult;
//      qy *= mult;
//      qz *= mult;
//      qw *= mult;
//    }

//    double roll, pitch, yaw;
//    // roll (x-axis rotation)
//    double sinr_cosp = 2 * (qw * qx + qy * qz);
//    double cosr_cosp = 1 - 2 * (qx * qx + qy * qy);
//    roll = std::atan2(sinr_cosp, cosr_cosp);

//    // pitch (y-axis rotation)
//    double sinp = 2 * (qw * qy - qz * qx);
//    if (std::abs(sinp) >= 1)
//    {
//      pitch = std::copysign(M_PI_2, sinp);  // use 90 degrees if out of range
//    }
//    else
//    {
//      pitch = std::asin(sinp);
//    }

//    // yaw (z-axis rotation)
//    double siny_cosp = 2 * (qw * qz + qx * qy);
//    double cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
//    yaw = std::atan2(siny_cosp, cosy_cosp);

//    PlotData& roll_data = plot_data.getOrCreateNumeric(prefix + "roll_deg");
//    roll_data.pushBack( {timestamp, RAD_TO_DEG*roll } );

//    PlotData& pitch_data = plot_data.getOrCreateNumeric(prefix + "pitch_deg");
//    pitch_data.pushBack( {timestamp, RAD_TO_DEG*pitch } );

//    PlotData& yaw_data = plot_data.getOrCreateNumeric(prefix + "yaw_deg");
//    yaw_data.pushBack( {timestamp, RAD_TO_DEG*yaw } );
//  }
//}
