#pragma once
#include <string_view>

namespace TestTypes
{
struct Point3D
{
  double x = 0;
  double y = 0;
  double z = 0;
};

struct Quaternion
{
  double w = 1;
  double x = 0;
  double y = 0;
  double z = 0;
};

struct Pose
{
  Point3D pos;
  Quaternion rot;
};

}  // end namespace TestTypes

namespace PseudoEigen
{

class Vector2d
{
  double _x = 0;
  double _y = 0;

public:
  Vector2d() = default;
  Vector2d(double x, double y) : _x(x), _y(y) {}

  const double& x() const { return _x; }
  const double& y() const { return _y; }

  double& x() { return _x; }
  double& y() { return _y; }
};

}  // namespace PseudoEigen

namespace TestTypes
{

template <typename AddField>
std::string_view TypeDefinition(Point3D& point, AddField& add)
{
  add("x", &point.x);
  add("y", &point.y);
  add("z", &point.z);
  return "Point3D";
}

//--------------------------------------------------------------
// We must specialize the function TypeDefinition
// This must be done in the same namespace of the original type

template <typename AddField>
std::string_view TypeDefinition(Quaternion& quat, AddField& add)
{
  add("w", &quat.w);
  add("x", &quat.x);
  add("y", &quat.y);
  add("z", &quat.z);
  return "Quaternion";
}

template <typename AddField>
std::string_view TypeDefinition(Pose& pose, AddField& add)
{
  add("position", &pose.pos);
  add("rotation", &pose.rot);
  return "Pose";
}

}  // end namespace TestTypes

namespace PseudoEigen
{
template <typename AddField>
std::string_view TypeDefinition(Vector2d& vect, AddField& add)
{
  add("x", &vect.x());
  add("y", &vect.y());
  return "Vector2d";
}

}  // end namespace PseudoEigen
