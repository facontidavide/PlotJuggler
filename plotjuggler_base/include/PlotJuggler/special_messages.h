#ifndef SPECIAL_MESSAGES_H
#define SPECIAL_MESSAGES_H

//These messages are exact equivalents of ROS messages


#include <vector>
#include <string>

namespace PJ::Msg
{

struct Time
{
  uint32_t sec;
  uint32_t nanosec;

  double toSec() const
  {
    return double(sec) + double(nanosec)*1e-9;
  }
};

struct Header
{
  uint32_t seq;
  PJ::Msg::Time stamp;
  std::string frame_id;
};
//--------------------
struct DiagnosticStatus
{
  uint8_t level;
  std::string name;
  std::string message;
  std::string hardware_id;
  std::vector<std::pair<std::string, std::string>> key_value;

  static const char* id() { return "diagnostic_msgs/DiagnosticStatus"; }
};

struct DiagnosticArray
{
  Header header;
  std::vector<DiagnosticStatus> status;

  static const char* id() { return "diagnostic_msgs/DiagnosticArray"; }
};

//--------------------
struct Vector3
{
  double x;
  double y;
  double z;

  static const char* id() { return "geometry_msgs/Vector3"; }
};

struct Quaternion
{
  double x;
  double y;
  double z;
  double w;

  static const char* id() { return "geometry_msgs/Quaternion"; }
};

struct RPY
{
  double roll;
  double pitch;
  double yaw;
};

RPY QuaternionToRPY(Quaternion q);

struct Transform
{
  Vector3 translation;
  Quaternion rotation;

  static const char* id() { return "geometry_msgs/Transform"; }
};

struct TransformStamped
{
  Header header;
  std::string child_frame_id;
  Transform transform;

  static const char* id() { return "geometry_msgs/TransformStamped"; }
};

struct TFMessage
{
  std::vector<TransformStamped> transforms;

  static const char* id() { return "tf2_msgs/TFMessage"; }
};
//--------------------

struct JointState
{
  Header header;
  std::vector<std::string> name;
  std::vector<double> position;
  std::vector<double> velocity;
  std::vector<double> effort;

  static const char* id() { return "sensor_msgs/JointState"; }
};

}

#endif // SPECIAL_MESSAGES_H
