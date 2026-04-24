#pragma once
// Arrow struct type definitions matching the Python SDK's __msco_pyarrow_struct__
// These are the inner (non-mixin) struct types used for nesting.

#include <arrow/api.h>

#include <memory>

namespace mosaico {

// --- Time / Header ---
inline std::shared_ptr<arrow::DataType> timeType() {
    return arrow::struct_({
        arrow::field("sec", arrow::int64()),
        arrow::field("nanosec", arrow::uint32()),
    });
}

inline std::shared_ptr<arrow::DataType> headerType() {
    return arrow::struct_({
        arrow::field("seq", arrow::uint32(), /*nullable=*/true),
        arrow::field("stamp", timeType(), /*nullable=*/true),
        arrow::field("frame_id", arrow::utf8(), /*nullable=*/true),
    });
}

// --- Geometry structs (inner/struct only, no mixins) ---
inline std::shared_ptr<arrow::DataType> vector2dStructType() {
    return arrow::struct_({
        arrow::field("x", arrow::float64(), /*nullable=*/true),
        arrow::field("y", arrow::float64(), /*nullable=*/true),
    });
}

inline std::shared_ptr<arrow::DataType> vector3dStructType() {
    return arrow::struct_({
        arrow::field("x", arrow::float64(), /*nullable=*/true),
        arrow::field("y", arrow::float64(), /*nullable=*/true),
        arrow::field("z", arrow::float64(), /*nullable=*/true),
    });
}

inline std::shared_ptr<arrow::DataType> quaternionStructType() {
    return arrow::struct_({
        arrow::field("x", arrow::float64(), /*nullable=*/true),
        arrow::field("y", arrow::float64(), /*nullable=*/true),
        arrow::field("z", arrow::float64(), /*nullable=*/true),
        arrow::field("w", arrow::float64(), /*nullable=*/true),
    });
}

// --- Full public types (with HeaderMixin + CovarianceMixin appended) ---
// Vector3d: struct{x,y,z} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> vector3dType() {
    return arrow::struct_({
        arrow::field("x", arrow::float64(), /*nullable=*/true),
        arrow::field("y", arrow::float64(), /*nullable=*/true),
        arrow::field("z", arrow::float64(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// Point3d: identical to Vector3d in schema
inline std::shared_ptr<arrow::DataType> point3dType() {
    return vector3dType();
}

// Quaternion: struct{x,y,z,w} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> quaternionType() {
    return arrow::struct_({
        arrow::field("x", arrow::float64(), /*nullable=*/true),
        arrow::field("y", arrow::float64(), /*nullable=*/true),
        arrow::field("z", arrow::float64(), /*nullable=*/true),
        arrow::field("w", arrow::float64(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// Pose: {position: Point3d, orientation: Quaternion} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> poseType() {
    return arrow::struct_({
        arrow::field("position", point3dType()),
        arrow::field("orientation", quaternionType()),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// Velocity: {linear: Vector3d, angular: Vector3d} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> velocityType() {
    return arrow::struct_({
        arrow::field("linear", vector3dType(), /*nullable=*/true),
        arrow::field("angular", vector3dType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// Acceleration: same schema as Velocity
inline std::shared_ptr<arrow::DataType> accelerationType() {
    return arrow::struct_({
        arrow::field("linear", vector3dType(), /*nullable=*/true),
        arrow::field("angular", vector3dType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// Transform: {translation: Vector3d, rotation: Quaternion, target_frame_id: string} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> transformType() {
    return arrow::struct_({
        arrow::field("translation", vector3dType()),
        arrow::field("rotation", quaternionType()),
        arrow::field("target_frame_id", arrow::utf8(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// ForceTorque: {force: Vector3d, torque: Vector3d} + header + covariance + covariance_type
inline std::shared_ptr<arrow::DataType> forceTorqueType() {
    return arrow::struct_({
        arrow::field("force", vector3dType()),
        arrow::field("torque", vector3dType()),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

// --- Sensor structs ---

// GPSStatus: {status: int8, service: uint16, satellites: int8, hdop: float64, vdop: float64} + header
inline std::shared_ptr<arrow::DataType> gpsStatusType() {
    return arrow::struct_({
        arrow::field("status", arrow::int8()),
        arrow::field("service", arrow::uint16()),
        arrow::field("satellites", arrow::int8(), /*nullable=*/true),
        arrow::field("hdop", arrow::float64(), /*nullable=*/true),
        arrow::field("vdop", arrow::float64(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

// ROI struct type
inline std::shared_ptr<arrow::DataType> roiType() {
    return arrow::struct_({
        arrow::field("offset", vector2dStructType()),
        arrow::field("height", arrow::uint32()),
        arrow::field("width", arrow::uint32()),
        arrow::field("do_rectify", arrow::boolean(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

}  // namespace mosaico
