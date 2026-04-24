#include "ontology/builders/gps_builders.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

namespace mosaico {

// ============================================================
// GpsBuilder
// GPS: {position: Point3d, velocity: Vector3d (nullable), status: GPSStatus (nullable)} + header
// The adapter maps: latitude->x, longitude->y, altitude->z
// position_covariance goes inside position's covariance field
// ============================================================
void GpsBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("position", point3dType()),
        arrow::field("velocity", vector3dType(), /*nullable=*/true),
        arrow::field("status", gpsStatusType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status GpsBuilder::appendRow(const FieldMap& fields) {
    // position: lat->x, lon->y, alt->z
    auto* pos_sb = structBuilder(2);
    {
        auto lat = getNumericAsDouble(fields, "latitude");
        auto lon = getNumericAsDouble(fields, "longitude");
        auto alt = getNumericAsDouble(fields, "altitude");
        ARROW_RETURN_NOT_OK(pos_sb->Append());
        auto* xb = static_cast<arrow::DoubleBuilder*>(pos_sb->field_builder(0));
        auto* yb = static_cast<arrow::DoubleBuilder*>(pos_sb->field_builder(1));
        auto* zb = static_cast<arrow::DoubleBuilder*>(pos_sb->field_builder(2));
        if (lat) ARROW_RETURN_NOT_OK(xb->Append(*lat)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
        if (lon) ARROW_RETURN_NOT_OK(yb->Append(*lon)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
        if (alt) ARROW_RETURN_NOT_OK(zb->Append(*alt)); else ARROW_RETURN_NOT_OK(zb->AppendNull());
        // header inside position: null
        auto* phb = static_cast<arrow::StructBuilder*>(pos_sb->field_builder(3));
        ARROW_RETURN_NOT_OK(phb->AppendNull());
        // covariance: from position_covariance
        auto* pcov = static_cast<arrow::ListBuilder*>(pos_sb->field_builder(4));
        auto* pct = static_cast<arrow::Int16Builder*>(pos_sb->field_builder(5));
        ARROW_RETURN_NOT_OK(appendCovariance(
            pcov, pct, fields, "position_covariance", "position_covariance_type"));
    }

    // velocity: nullable — not typically in NavSatFix
    auto* vel_sb = structBuilder(3);
    ARROW_RETURN_NOT_OK(vel_sb->AppendNull());

    // status: struct {status: int8, service: uint16, satellites: int8, hdop: float64, vdop: float64, header}
    auto* stat_sb = structBuilder(4);
    bool has_status = fields.fields.count("status.status") || fields.fields.count("status.service");
    if (has_status) {
        ARROW_RETURN_NOT_OK(stat_sb->Append());
        auto status_val = getInt8(fields, "status.status");
        auto* sb0 = static_cast<arrow::Int8Builder*>(stat_sb->field_builder(0));
        if (status_val) ARROW_RETURN_NOT_OK(sb0->Append(*status_val));
        else ARROW_RETURN_NOT_OK(sb0->AppendNull());

        auto service_val = getUint16(fields, "status.service");
        auto* sb1 = static_cast<arrow::UInt16Builder*>(stat_sb->field_builder(1));
        if (service_val) ARROW_RETURN_NOT_OK(sb1->Append(*service_val));
        else ARROW_RETURN_NOT_OK(sb1->AppendNull());

        // satellites (nullable)
        auto* sb2 = static_cast<arrow::Int8Builder*>(stat_sb->field_builder(2));
        ARROW_RETURN_NOT_OK(sb2->AppendNull());
        // hdop (nullable)
        auto* sb3 = static_cast<arrow::DoubleBuilder*>(stat_sb->field_builder(3));
        ARROW_RETURN_NOT_OK(sb3->AppendNull());
        // vdop (nullable)
        auto* sb4 = static_cast<arrow::DoubleBuilder*>(stat_sb->field_builder(4));
        ARROW_RETURN_NOT_OK(sb4->AppendNull());
        // header inside status: null
        auto* shb = static_cast<arrow::StructBuilder*>(stat_sb->field_builder(5));
        ARROW_RETURN_NOT_OK(shb->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(stat_sb->AppendNull());
    }

    // header
    auto* hb = structBuilder(5);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    return arrow::Status::OK();
}

// ============================================================
// GpsStatusBuilder
// Standalone GPSStatus: {status, service, satellites, hdop, vdop} + header
// ============================================================
void GpsStatusBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("status", arrow::int8()),
        arrow::field("service", arrow::uint16()),
        arrow::field("satellites", arrow::int8(), /*nullable=*/true),
        arrow::field("hdop", arrow::float64(), /*nullable=*/true),
        arrow::field("vdop", arrow::float64(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status GpsStatusBuilder::appendRow(const FieldMap& fields) {
    auto status_val = getInt8(fields, "status");
    auto* sb = batch_builder_->GetFieldAs<arrow::Int8Builder>(2);
    if (status_val) ARROW_RETURN_NOT_OK(sb->Append(*status_val));
    else ARROW_RETURN_NOT_OK(sb->AppendNull());

    auto service_val = getUint16(fields, "service");
    auto* svb = batch_builder_->GetFieldAs<arrow::UInt16Builder>(3);
    if (service_val) ARROW_RETURN_NOT_OK(svb->Append(*service_val));
    else ARROW_RETURN_NOT_OK(svb->AppendNull());

    // satellites, hdop, vdop
    auto* sat = batch_builder_->GetFieldAs<arrow::Int8Builder>(4);
    ARROW_RETURN_NOT_OK(sat->AppendNull());
    auto* hdop = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(5);
    ARROW_RETURN_NOT_OK(hdop->AppendNull());
    auto* vdop = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(6);
    ARROW_RETURN_NOT_OK(vdop->AppendNull());

    auto* hb = structBuilder(7);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));

    return arrow::Status::OK();
}

}  // namespace mosaico
