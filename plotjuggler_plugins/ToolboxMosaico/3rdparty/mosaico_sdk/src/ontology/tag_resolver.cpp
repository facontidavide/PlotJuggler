#include "ontology/tag_resolver.hpp"

namespace mosaico {

void TagResolver::map(const std::string& source_tag, const std::string& ontology_tag) {
    map_[source_tag] = ontology_tag;
}

std::optional<std::string> TagResolver::resolve(const std::string& source_tag) const {
    auto it = map_.find(source_tag);
    if (it == map_.end()) {
        return std::nullopt;
    }
    return it->second;
}

static void mapRosType(TagResolver& r, const std::string& pkg,
                       const std::string& type, const std::string& tag) {
    r.map(pkg + "/msg/" + type, tag);    // ROS2
    r.map(pkg + "/" + type, tag);         // ROS1
    r.map(pkg + ".msg." + type, tag);     // Protobuf
}

TagResolver createRosTagResolver() {
    TagResolver r;

    // sensor_msgs
    // Tags must match Python SDK's camel_to_snake(ClassName) convention
    mapRosType(r, "sensor_msgs", "Imu", "imu");
    mapRosType(r, "sensor_msgs", "NavSatFix", "gps");
    mapRosType(r, "sensor_msgs", "Image", "image");
    mapRosType(r, "sensor_msgs", "CompressedImage", "compressed_image");
    mapRosType(r, "sensor_msgs", "CameraInfo", "camera_info");
    mapRosType(r, "sensor_msgs", "BatteryState", "battery_state");
    mapRosType(r, "sensor_msgs", "JointState", "robot_joint");
    mapRosType(r, "sensor_msgs", "RegionOfInterest", "roi");

    // nmea_msgs
    mapRosType(r, "nmea_msgs", "Sentence", "nmea_sentence");

    // geometry_msgs base types
    mapRosType(r, "geometry_msgs", "Vector3", "vector3d");
    mapRosType(r, "geometry_msgs", "Vector3Stamped", "vector3d");
    mapRosType(r, "geometry_msgs", "Point", "point3d");
    mapRosType(r, "geometry_msgs", "PointStamped", "point3d");
    mapRosType(r, "geometry_msgs", "Quaternion", "quaternion");
    mapRosType(r, "geometry_msgs", "QuaternionStamped", "quaternion");

    // geometry_msgs compound types (4 variants each)
    mapRosType(r, "geometry_msgs", "Pose", "pose");
    mapRosType(r, "geometry_msgs", "PoseStamped", "pose");
    mapRosType(r, "geometry_msgs", "PoseWithCovariance", "pose");
    mapRosType(r, "geometry_msgs", "PoseWithCovarianceStamped", "pose");

    mapRosType(r, "geometry_msgs", "Twist", "velocity");
    mapRosType(r, "geometry_msgs", "TwistStamped", "velocity");
    mapRosType(r, "geometry_msgs", "TwistWithCovariance", "velocity");
    mapRosType(r, "geometry_msgs", "TwistWithCovarianceStamped", "velocity");

    mapRosType(r, "geometry_msgs", "Accel", "acceleration");
    mapRosType(r, "geometry_msgs", "AccelStamped", "acceleration");
    mapRosType(r, "geometry_msgs", "AccelWithCovariance", "acceleration");
    mapRosType(r, "geometry_msgs", "AccelWithCovarianceStamped", "acceleration");

    mapRosType(r, "geometry_msgs", "Transform", "transform");
    mapRosType(r, "geometry_msgs", "TransformStamped", "transform");

    mapRosType(r, "geometry_msgs", "Wrench", "force_torque");
    mapRosType(r, "geometry_msgs", "WrenchStamped", "force_torque");

    // nav_msgs
    mapRosType(r, "nav_msgs", "Odometry", "motion_state");

    // tf2_msgs
    mapRosType(r, "tf2_msgs", "TFMessage", "frame_transform");

    // std_msgs — tags match Python SDK: camel_to_snake of Mosaico model class names
    mapRosType(r, "std_msgs", "String", "string");
    mapRosType(r, "std_msgs", "Bool", "boolean");
    mapRosType(r, "std_msgs", "Int8", "integer8");
    mapRosType(r, "std_msgs", "Int16", "integer16");
    mapRosType(r, "std_msgs", "Int32", "integer32");
    mapRosType(r, "std_msgs", "Int64", "integer64");
    mapRosType(r, "std_msgs", "UInt8", "unsigned8");
    mapRosType(r, "std_msgs", "UInt16", "unsigned16");
    mapRosType(r, "std_msgs", "UInt32", "unsigned32");
    mapRosType(r, "std_msgs", "UInt64", "unsigned64");
    mapRosType(r, "std_msgs", "Float32", "floating32");
    mapRosType(r, "std_msgs", "Float64", "floating64");

    return r;
}

}  // namespace mosaico
