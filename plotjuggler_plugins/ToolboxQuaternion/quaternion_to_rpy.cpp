#include "quaternion_to_rpy.h"
#include <array>
#include <math.h>

QuaternionToRollPitchYaw::QuaternionToRollPitchYaw()
{
  reset();
}

void QuaternionToRollPitchYaw::reset()
{
  _prev_roll = 0;
  _prev_yaw = 0;
  _prev_pitch = 0;
  _roll_offset = 0;
  _pitch_offset = 0;
  _yaw_offset = 0;
  _last_timestamp = std::numeric_limits<double>::lowest();
}

void QuaternionToRollPitchYaw::calculate()
{
  auto& data_x = *_src_vector[0];
  auto& data_y = *_src_vector[1];
  auto& data_z = *_src_vector[2];
  auto& data_w = *_src_vector[3];

  auto& data_roll = *_dst_vector[0];
  auto& data_pitch = *_dst_vector[1];
  auto& data_yaw = *_dst_vector[2];

  data_roll.setMaximumRangeX(data_x.maximumRangeX());
  data_pitch.setMaximumRangeX(data_x.maximumRangeX());
  data_yaw.setMaximumRangeX(data_x.maximumRangeX());

  data_roll.clear();
  data_pitch.clear();
  data_yaw.clear();

  if (data_x.size() == 0 || data_x.size() != data_y.size() || data_y.size() != data_z.size() ||
      data_z.size() != data_w.size())
  {
    return;
  }

  int pos = data_x.getIndexFromX(_last_timestamp);
  size_t index = pos < 0 ? 0 : static_cast<size_t>(pos);

  while (index < data_x.size())
  {
    auto& point_x = data_x.at(index);
    double timestamp = point_x.x;
    double q_x = point_x.y;
    double q_y = data_y.at(index).y;
    double q_z = data_z.at(index).y;
    double q_w = data_w.at(index).y;

    if (timestamp >= _last_timestamp)
    {
      std::array<double, 3> RPY;
      calculateNextPoint(index, { q_x, q_y, q_z, q_w }, RPY);

      data_roll.pushBack({ timestamp, _scale * (RPY[0] + _roll_offset) });
      data_pitch.pushBack({ timestamp, _scale * (RPY[1] + _pitch_offset) });
      data_yaw.pushBack({ timestamp, _scale * (RPY[2] + _yaw_offset) });

      _last_timestamp = timestamp;
    }
    index++;
  }
}

void QuaternionToRollPitchYaw::calculateNextPoint(size_t index, const std::array<double, 4>& quat,
                                                  std::array<double, 3>& rpy)
{
  double q_x = quat[0];
  double q_y = quat[1];
  double q_z = quat[2];
  double q_w = quat[3];

  double quat_norm2 = (q_w * q_w) + (q_x * q_x) + (q_y * q_y) + (q_z * q_z);
  if (std::abs(quat_norm2 - 1.0) > std::numeric_limits<double>::epsilon())
  {
    double mult = 1.0 / std::sqrt(quat_norm2);
    q_x *= mult;
    q_y *= mult;
    q_z *= mult;
    q_w *= mult;
  }
  double roll, pitch, yaw;
  // roll (x-axis rotation)
  double sinr_cosp = 2 * (q_w * q_x + q_y * q_z);
  double cosr_cosp = 1 - 2 * (q_x * q_x + q_y * q_y);
  roll = std::atan2(sinr_cosp, cosr_cosp);

  // pitch (y-axis rotation)
  double sinp = 2 * (q_w * q_y - q_z * q_x);
  if (std::abs(sinp) >= 1)
  {
    pitch = std::copysign(M_PI_2, sinp);  // use 90 degrees if out of range
  }
  else
  {
    pitch = std::asin(sinp);
  }
  // yaw (z-axis rotation)
  double siny_cosp = 2 * (q_w * q_z + q_x * q_y);
  double cosy_cosp = 1 - 2 * (q_y * q_y + q_z * q_z);
  yaw = std::atan2(siny_cosp, cosy_cosp);

  const double WRAP_ANGLE = M_PI * 2.0;
  const double WRAP_THRESHOLD = M_PI * 1.95;

  //--------- wrap ------
  if (index != 0 && _wrap)
  {
    if ((roll - _prev_roll) > WRAP_THRESHOLD)
    {
      _roll_offset -= WRAP_ANGLE;
    }
    else if ((_prev_roll - roll) > WRAP_THRESHOLD)
    {
      _roll_offset += WRAP_ANGLE;
    }

    if ((pitch - _prev_pitch) > WRAP_THRESHOLD)
    {
      _pitch_offset -= WRAP_ANGLE;
    }
    else if ((_prev_pitch - pitch) > WRAP_THRESHOLD)
    {
      _pitch_offset += WRAP_ANGLE;
    }

    if ((yaw - _prev_yaw) > WRAP_THRESHOLD)
    {
      _yaw_offset -= WRAP_ANGLE;
    }
    else if ((_prev_yaw - yaw) > WRAP_THRESHOLD)
    {
      _yaw_offset += WRAP_ANGLE;
    }
  }

  _prev_pitch = pitch;
  _prev_roll = roll;
  _prev_yaw = yaw;

  rpy = { roll, pitch, yaw };
}
