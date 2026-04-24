#include "ontology/builders/geometry_builders.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

namespace mosaico {

// ============================================================
// Vector3dBuilder
// Schema: timestamp_ns, recording_timestamp_ns, x, y, z,
//         header, covariance, covariance_type
// ============================================================
void Vector3dBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("x", arrow::float64()),
        arrow::field("y", arrow::float64()),
        arrow::field("z", arrow::float64()),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

arrow::Status Vector3dBuilder::appendRow(const FieldMap& fields) {
    // Unwrap: Vector3Stamped has "vector.x", plain has "x"
    auto prefix = resolvePrefix(fields, "x", {"vector.", ""});

    auto x = getNumericAsDouble(fields, prefix + "x");
    auto y = getNumericAsDouble(fields, prefix + "y");
    auto z = getNumericAsDouble(fields, prefix + "z");

    auto* xb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(2);
    auto* yb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(3);
    auto* zb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(4);
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());

    // header
    auto* hb = structBuilder(5);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    // covariance
    auto* cov_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(6));
    auto* cov_type_b = batch_builder_->GetFieldAs<arrow::Int16Builder>(7);
    ARROW_RETURN_NOT_OK(appendCovariance(cov_b, cov_type_b, fields, "covariance"));

    return arrow::Status::OK();
}

// ============================================================
// Point3dBuilder — identical schema to Vector3d
// ============================================================
void Point3dBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("x", arrow::float64()),
        arrow::field("y", arrow::float64()),
        arrow::field("z", arrow::float64()),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

arrow::Status Point3dBuilder::appendRow(const FieldMap& fields) {
    auto prefix = resolvePrefix(fields, "x", {"point.", ""});

    auto x = getNumericAsDouble(fields, prefix + "x");
    auto y = getNumericAsDouble(fields, prefix + "y");
    auto z = getNumericAsDouble(fields, prefix + "z");

    auto* xb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(2);
    auto* yb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(3);
    auto* zb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(4);
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());

    auto* hb = structBuilder(5);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    auto* cov_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(6));
    auto* cov_type_b = batch_builder_->GetFieldAs<arrow::Int16Builder>(7);
    ARROW_RETURN_NOT_OK(appendCovariance(cov_b, cov_type_b, fields, "covariance"));

    return arrow::Status::OK();
}

// ============================================================
// QuaternionBuilder
// Schema: timestamp_ns, recording_timestamp_ns, x, y, z, w,
//         header, covariance, covariance_type
// ============================================================
void QuaternionBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("x", arrow::float64()),
        arrow::field("y", arrow::float64()),
        arrow::field("z", arrow::float64()),
        arrow::field("w", arrow::float64()),
        arrow::field("header", headerType(), /*nullable=*/true),
        arrow::field("covariance", arrow::list(arrow::float64()), /*nullable=*/true),
        arrow::field("covariance_type", arrow::int16(), /*nullable=*/true),
    });
}

arrow::Status QuaternionBuilder::appendRow(const FieldMap& fields) {
    auto prefix = resolvePrefix(fields, "x", {"quaternion.", ""});

    auto x = getNumericAsDouble(fields, prefix + "x");
    auto y = getNumericAsDouble(fields, prefix + "y");
    auto z = getNumericAsDouble(fields, prefix + "z");
    auto w = getNumericAsDouble(fields, prefix + "w");

    auto* xb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(2);
    auto* yb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(3);
    auto* zb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(4);
    auto* wb = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(5);
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());
    if (w) ARROW_RETURN_NOT_OK(wb->Append(*w)); else ARROW_RETURN_NOT_OK(wb->AppendNull());

    auto* hb = structBuilder(6);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    auto* cov_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(7));
    auto* cov_type_b = batch_builder_->GetFieldAs<arrow::Int16Builder>(8);
    ARROW_RETURN_NOT_OK(appendCovariance(cov_b, cov_type_b, fields, "covariance"));

    return arrow::Status::OK();
}

}  // namespace mosaico
