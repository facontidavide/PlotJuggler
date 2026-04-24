#include "ontology/ontology_registry.hpp"

#include "ontology/builders/geometry_builders.hpp"
#include "ontology/builders/compound_geometry_builders.hpp"
#include "ontology/builders/imu_builder.hpp"
#include "ontology/builders/gps_builders.hpp"
#include "ontology/builders/nav_builders.hpp"
#include "ontology/builders/sensor_builders.hpp"
#include "ontology/builders/std_type_builder.hpp"

namespace mosaico {

OntologyRegistry::OntologyRegistry() {
    auto add = [this](const std::string& tag,
                      std::unique_ptr<OntologyBuilder> builder) {
        builders_.emplace(tag, std::move(builder));
    };

    // Geometry primitives — keys match Python SDK's camel_to_snake(ClassName)
    add("vector3d",        std::make_unique<Vector3dBuilder>());
    add("point3d",         std::make_unique<Point3dBuilder>());
    add("quaternion",      std::make_unique<QuaternionBuilder>());

    // Compound geometry
    add("pose",            std::make_unique<PoseBuilder>());
    add("velocity",        std::make_unique<VelocityBuilder>());
    add("acceleration",    std::make_unique<AccelerationBuilder>());
    add("transform",       std::make_unique<TransformBuilder>());
    add("force_torque",    std::make_unique<ForceTorqueBuilder>());

    // IMU and GPS
    add("imu",             std::make_unique<ImuBuilder>());
    add("gps",             std::make_unique<GpsBuilder>());
    add("gps_status",      std::make_unique<GpsStatusBuilder>());

    // Navigation
    add("motion_state",    std::make_unique<MotionStateBuilder>());
    add("frame_transform", std::make_unique<FrameTransformBuilder>());

    // Sensors
    add("image",            std::make_unique<ImageBuilder>());
    add("compressed_image", std::make_unique<CompressedImageBuilder>());
    add("camera_info",      std::make_unique<CameraInfoBuilder>());
    add("battery_state",    std::make_unique<BatteryStateBuilder>());
    add("robot_joint",      std::make_unique<RobotJointBuilder>());
    add("nmea_sentence",    std::make_unique<NmeaSentenceBuilder>());
    add("roi",              std::make_unique<RoiBuilder>());

    // Std types — tags match Python SDK's camel_to_snake convention
    add("string",     std::make_unique<StdTypeBuilder<arrow::StringType>>("string"));
    add("boolean",    std::make_unique<StdTypeBuilder<arrow::BooleanType>>("boolean"));
    add("integer8",   std::make_unique<StdTypeBuilder<arrow::Int8Type>>("integer8"));
    add("integer16",  std::make_unique<StdTypeBuilder<arrow::Int16Type>>("integer16"));
    add("integer32",  std::make_unique<StdTypeBuilder<arrow::Int32Type>>("integer32"));
    add("integer64",  std::make_unique<StdTypeBuilder<arrow::Int64Type>>("integer64"));
    add("unsigned8",  std::make_unique<StdTypeBuilder<arrow::UInt8Type>>("unsigned8"));
    add("unsigned16", std::make_unique<StdTypeBuilder<arrow::UInt16Type>>("unsigned16"));
    add("unsigned32", std::make_unique<StdTypeBuilder<arrow::UInt32Type>>("unsigned32"));
    add("unsigned64", std::make_unique<StdTypeBuilder<arrow::UInt64Type>>("unsigned64"));
    add("floating32", std::make_unique<StdTypeBuilder<arrow::FloatType>>("floating32"));
    add("floating64", std::make_unique<StdTypeBuilder<arrow::DoubleType>>("floating64"));
}

OntologyBuilder* OntologyRegistry::find(const std::string& tag) {
    auto it = builders_.find(tag);
    return it == builders_.end() ? nullptr : it->second.get();
}

}  // namespace mosaico
