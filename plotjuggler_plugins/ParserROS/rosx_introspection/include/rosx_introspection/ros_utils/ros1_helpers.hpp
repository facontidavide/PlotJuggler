#pragma once

#include <ros/message_traits.h>
#include "rosx_introspection/ros_field.hpp"

namespace RosMsgParser
{

template <typename T>
inline std::string GetMessageDefinition()
{
  return ros::message_traits::Definition<T>::value();
}

template <typename T>
inline ROSType GetMessageType()
{
  return ROSType(ros::message_traits::DataType<sensor_msgs::JointState>::value());
}

template <typename T>
inline std::vector<uint8_t> BuildMessageBuffer(const T& msg)
{
  std::vector<uint8_t> buffer(ros::serialization::serializationLength(msg));
  ros::serialization::OStream stream(buffer.data(), buffer.size());
  ros::serialization::Serializer<T>::write(stream, msg);
  return buffer;
}

}  // namespace RosMsgParser
