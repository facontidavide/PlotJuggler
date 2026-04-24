#include "ontology/builders/nav_builders.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

namespace mosaico {

// ============================================================
// MotionStateBuilder (Odometry)
// MotionState: {pose: Pose, velocity: Velocity, target_frame_id: string,
//               acceleration: Acceleration (nullable)}
//              + header + covariance + covariance_type
// ============================================================
void MotionStateBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("pose", poseType()),
        arrow::field("velocity", velocityType()),
        arrow::field("target_frame_id", arrow::utf8()),
        arrow::field("acceleration", accelerationType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

arrow::Status MotionStateBuilder::appendRow(const FieldMap& fields) {
    // pose: Odometry has "pose.pose.position.x" or "pose.position.x"
    auto pose_prefix = resolvePrefix(fields, "position.x", {"pose.pose.", "pose."});

    // Build pose struct: {position, orientation, header, covariance, covariance_type}
    auto* pose_sb = structBuilder(2);
    ARROW_RETURN_NOT_OK(pose_sb->Append());
    // position inside pose
    auto* pos_sb = static_cast<arrow::StructBuilder*>(pose_sb->field_builder(0));
    ARROW_RETURN_NOT_OK(appendPoint3d(pos_sb, fields, pose_prefix + "position."));
    // orientation inside pose
    auto* ori_sb = static_cast<arrow::StructBuilder*>(pose_sb->field_builder(1));
    ARROW_RETURN_NOT_OK(appendQuaternion(ori_sb, fields, pose_prefix + "orientation."));
    // pose header
    auto* pose_hb = static_cast<arrow::StructBuilder*>(pose_sb->field_builder(2));
    ARROW_RETURN_NOT_OK(pose_hb->AppendNull());
    // pose covariance
    auto* pose_cov = static_cast<arrow::ListBuilder*>(pose_sb->field_builder(3));
    auto* pose_ct = static_cast<arrow::Int16Builder*>(pose_sb->field_builder(4));
    ARROW_RETURN_NOT_OK(appendCovariance(pose_cov, pose_ct, fields, "pose.covariance"));

    // velocity: Odometry has "twist.twist.linear.x" or "twist.linear.x"
    auto twist_prefix = resolvePrefix(fields, "linear.x", {"twist.twist.", "twist."});
    auto* vel_sb = structBuilder(3);
    ARROW_RETURN_NOT_OK(vel_sb->Append());
    auto* lin_sb = static_cast<arrow::StructBuilder*>(vel_sb->field_builder(0));
    ARROW_RETURN_NOT_OK(appendVector3d(lin_sb, fields, twist_prefix + "linear.", "", true));
    auto* ang_sb = static_cast<arrow::StructBuilder*>(vel_sb->field_builder(1));
    ARROW_RETURN_NOT_OK(appendVector3d(ang_sb, fields, twist_prefix + "angular.", "", true));
    // velocity header
    auto* vel_hb = static_cast<arrow::StructBuilder*>(vel_sb->field_builder(2));
    ARROW_RETURN_NOT_OK(vel_hb->AppendNull());
    // velocity covariance
    auto* vel_cov = static_cast<arrow::ListBuilder*>(vel_sb->field_builder(3));
    auto* vel_ct = static_cast<arrow::Int16Builder*>(vel_sb->field_builder(4));
    ARROW_RETURN_NOT_OK(appendCovariance(vel_cov, vel_ct, fields, "twist.covariance"));

    // target_frame_id
    auto* tfid_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(4);
    auto tfid = getString(fields, "child_frame_id");
    if (tfid) ARROW_RETURN_NOT_OK(tfid_b->Append(*tfid));
    else ARROW_RETURN_NOT_OK(tfid_b->Append(""));

    // acceleration: nullable, not in Odometry
    auto* accel_sb = structBuilder(5);
    ARROW_RETURN_NOT_OK(accel_sb->AppendNull());

    // header
    auto* hb = structBuilder(6);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    // covariance
    auto* cov_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(7));
    auto* cov_type_b = batch_builder_->GetFieldAs<arrow::Int16Builder>(8);
    ARROW_RETURN_NOT_OK(appendCovariance(cov_b, cov_type_b, fields, "covariance"));

    return arrow::Status::OK();
}

// ============================================================
// FrameTransformBuilder (TFMessage)
// The FrameTransform schema has a single list<Transform> field "transforms"
// but for the C++ builder we produce one row per transform (expanding the list).
// Each row has the Transform schema columns.
// ============================================================
void FrameTransformBuilder::initSchema() {
    // For the flat representation, each row is a Transform
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("translation", vector3dType()),
        arrow::field("rotation", quaternionType()),
        arrow::field("target_frame_id", arrow::utf8(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

arrow::Status FrameTransformBuilder::append(const FieldMap& fields) {
    if (!schema_) {
        initSchema();
        ARROW_ASSIGN_OR_RAISE(batch_builder_,
            arrow::RecordBatchBuilder::Make(schema_, arrow::default_memory_pool()));
    }

    // Get the "transforms" array
    const auto* transforms = getFieldMapArray(fields, "transforms");
    if (!transforms || transforms->empty()) {
        // Fall back: maybe it's a single TransformStamped
        auto [ts, rec_ts] = extractTimestamps(fields);
        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(0)->Append(ts));
        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(1)->Append(rec_ts));
        ARROW_RETURN_NOT_OK(appendRow(fields));
        ++row_count_;
        return arrow::Status::OK();
    }

    auto [base_ts, base_rec_ts] = extractTimestamps(fields);

    for (const auto& tf_fm : *transforms) {
        // Each sub-FieldMap is a TransformStamped
        auto [ts, rec_ts] = extractTimestamps(tf_fm);
        if (ts == 0) ts = base_ts;
        if (rec_ts == 0) rec_ts = base_rec_ts;

        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(0)->Append(ts));
        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(1)->Append(rec_ts));
        ARROW_RETURN_NOT_OK(appendRow(tf_fm));
        ++row_count_;
    }

    return arrow::Status::OK();
}

arrow::Status FrameTransformBuilder::appendRow(const FieldMap& fields) {
    auto prefix = resolvePrefix(fields, "translation.x", {"transform.", ""});

    auto* trans_sb = structBuilder(2);
    ARROW_RETURN_NOT_OK(appendVector3d(trans_sb, fields, prefix + "translation."));

    auto* rot_sb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendQuaternion(rot_sb, fields, prefix + "rotation."));

    auto* tfid_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(4);
    auto tfid = getString(fields, "child_frame_id");
    if (tfid) ARROW_RETURN_NOT_OK(tfid_b->Append(*tfid));
    else ARROW_RETURN_NOT_OK(tfid_b->AppendNull());

    auto* hb = structBuilder(5);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    auto* cov_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(6));
    auto* cov_type_b = batch_builder_->GetFieldAs<arrow::Int16Builder>(7);
    ARROW_RETURN_NOT_OK(appendCovariance(cov_b, cov_type_b, fields, "covariance"));

    return arrow::Status::OK();
}

}  // namespace mosaico
