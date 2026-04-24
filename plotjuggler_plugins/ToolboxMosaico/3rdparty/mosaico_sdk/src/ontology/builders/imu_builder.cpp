#include "ontology/builders/imu_builder.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

namespace mosaico {

// IMU schema: timestamp_ns, recording_timestamp_ns,
//   acceleration (Vector3d), angular_velocity (Vector3d),
//   orientation (Quaternion, nullable),
//   header, covariance (not used at top level for IMU — covariance is inside sub-structs)
// But the Python model adds HeaderMixin (no CovarianceMixin per the Python class hierarchy
// for IMU: it's Serializable + HeaderMixin only).
// Actually: IMU is Serializable, HeaderMixin — NO CovarianceMixin at top level.

void ImuBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("acceleration", vector3dType()),
        arrow::field("angular_velocity", vector3dType()),
        arrow::field("orientation", quaternionType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status ImuBuilder::appendRow(const FieldMap& fields) {
    // acceleration — from "linear_acceleration." prefix
    auto* accel_sb = structBuilder(2);
    ARROW_RETURN_NOT_OK(appendVector3d(
        accel_sb, fields, "linear_acceleration.",
        "linear_acceleration_covariance"));

    // angular_velocity — from "angular_velocity." prefix
    auto* angvel_sb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendVector3d(
        angvel_sb, fields, "angular_velocity.",
        "angular_velocity_covariance"));

    // orientation — nullable, check covariance[0] != -1
    auto* ori_sb = structBuilder(4);
    const auto* ori_cov = getDoubleArray(fields, "orientation_covariance");
    bool orientation_available = ori_cov && !ori_cov->empty() && (*ori_cov)[0] != -1.0;
    if (orientation_available &&
        (fields.fields.count("orientation.x") || fields.fields.count("orientation.w"))) {
        ARROW_RETURN_NOT_OK(appendQuaternion(
            ori_sb, fields, "orientation.",
            (ori_cov && isValidCovariance(*ori_cov)) ? "orientation_covariance" : ""));
    } else {
        ARROW_RETURN_NOT_OK(ori_sb->AppendNull());
    }

    // header
    auto* hb = structBuilder(5);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    return arrow::Status::OK();
}

}  // namespace mosaico
