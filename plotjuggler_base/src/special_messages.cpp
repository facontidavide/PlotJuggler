#include "PlotJuggler/special_messages.h"
#include <cmath>

PJ::Msg::RPY PJ::Msg::QuaternionToRPY(PJ::Msg::Quaternion q)
{
  RPY rpy;

  double quat_norm2 = (q.w * q.w) + (q.x * q.x) + (q.y * q.y) + (q.z * q.z);
  if (std::abs(quat_norm2 - 1.0) > std::numeric_limits<double>::epsilon())
  {
    double mult = 1.0 / std::sqrt(quat_norm2);
    q.x *= mult;
    q.y *= mult;
    q.z *= mult;
    q.w *= mult;
  }

  // roll (x-axis rotation)
  double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
  double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
  rpy.roll = std::atan2(sinr_cosp, cosr_cosp);

  // pitch (y-axis rotation)
  double sinp = 2 * (q.w * q.y - q.z * q.x);
  if (std::abs(sinp) >= 1)
  {
    rpy.pitch = std::copysign(M_PI_2, sinp);  // use 90 degrees if out of range
  }
  else
  {
    rpy.pitch = std::asin(sinp);
  }

  // yaw (z-axis rotation)
  double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
  double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
  rpy.yaw = std::atan2(siny_cosp, cosy_cosp);

  return rpy;
}
