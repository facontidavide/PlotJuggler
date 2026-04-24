#include "ontology/builders/sensor_builders.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

namespace mosaico {

// ============================================================
// ImageBuilder
// Image: {data: binary, format: string, width: int32, height: int32,
//         stride: int32, encoding: string, is_bigendian: bool} + header
// ============================================================
void ImageBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("data", arrow::binary()),
        arrow::field("format", arrow::utf8()),
        arrow::field("width", arrow::int32()),
        arrow::field("height", arrow::int32()),
        arrow::field("stride", arrow::int32()),
        arrow::field("encoding", arrow::utf8()),
        arrow::field("is_bigendian", arrow::boolean(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status ImageBuilder::appendRow(const FieldMap& fields) {
    // data (binary)
    auto* data_b = batch_builder_->GetFieldAs<arrow::BinaryBuilder>(2);
    const auto* raw_data = getByteArray(fields, "data");
    if (raw_data) {
        ARROW_RETURN_NOT_OK(data_b->Append(
            reinterpret_cast<const char*>(raw_data->data()),
            static_cast<int32_t>(raw_data->size())));
    } else {
        ARROW_RETURN_NOT_OK(data_b->Append("", 0));
    }

    // format — for raw Image, we store "raw" (the SDK does PNG compression in Python)
    auto* fmt_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(3);
    auto fmt = getString(fields, "format");
    ARROW_RETURN_NOT_OK(fmt_b->Append(fmt.value_or("raw")));

    // width
    auto* wb = batch_builder_->GetFieldAs<arrow::Int32Builder>(4);
    auto width = getInt32(fields, "width");
    if (width) ARROW_RETURN_NOT_OK(wb->Append(*width)); else ARROW_RETURN_NOT_OK(wb->Append(0));

    // height
    auto* hb = batch_builder_->GetFieldAs<arrow::Int32Builder>(5);
    auto height = getInt32(fields, "height");
    if (height) ARROW_RETURN_NOT_OK(hb->Append(*height)); else ARROW_RETURN_NOT_OK(hb->Append(0));

    // stride (step)
    auto* sb = batch_builder_->GetFieldAs<arrow::Int32Builder>(6);
    auto step = getInt32(fields, "step");
    if (step) ARROW_RETURN_NOT_OK(sb->Append(*step)); else ARROW_RETURN_NOT_OK(sb->Append(0));

    // encoding
    auto* eb = batch_builder_->GetFieldAs<arrow::StringBuilder>(7);
    auto enc = getString(fields, "encoding");
    ARROW_RETURN_NOT_OK(eb->Append(enc.value_or("")));

    // is_bigendian
    auto* beb = batch_builder_->GetFieldAs<arrow::BooleanBuilder>(8);
    auto be = getBool(fields, "is_bigendian");
    if (be) ARROW_RETURN_NOT_OK(beb->Append(*be)); else ARROW_RETURN_NOT_OK(beb->AppendNull());

    // header
    auto* header_b = structBuilder(9);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// CompressedImageBuilder
// CompressedImage: {data: binary, format: string} + header
// ============================================================
void CompressedImageBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("data", arrow::binary()),
        arrow::field("format", arrow::utf8()),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status CompressedImageBuilder::appendRow(const FieldMap& fields) {
    auto* data_b = batch_builder_->GetFieldAs<arrow::BinaryBuilder>(2);
    const auto* raw_data = getByteArray(fields, "data");
    if (raw_data) {
        ARROW_RETURN_NOT_OK(data_b->Append(
            reinterpret_cast<const char*>(raw_data->data()),
            static_cast<int32_t>(raw_data->size())));
    } else {
        ARROW_RETURN_NOT_OK(data_b->Append("", 0));
    }

    auto* fmt_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(3);
    auto fmt = getString(fields, "format");
    ARROW_RETURN_NOT_OK(fmt_b->Append(fmt.value_or("")));

    auto* header_b = structBuilder(4);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// CameraInfoBuilder
// CameraInfo: {height: uint32, width: uint32, distortion_model: string,
//   distortion_parameters: list<float64>, intrinsic_parameters: fixed_size_list<float64, 9>,
//   rectification_parameters: fixed_size_list<float64, 9>,
//   projection_parameters: fixed_size_list<float64, 12>,
//   binning: Vector2d (nullable), roi: ROI (nullable)} + header
// ============================================================
void CameraInfoBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("height", arrow::uint32()),
        arrow::field("width", arrow::uint32()),
        arrow::field("distortion_model", arrow::utf8()),
        arrow::field("distortion_parameters", arrow::list(arrow::float64())),
        arrow::field("intrinsic_parameters", arrow::fixed_size_list(arrow::float64(), 9)),
        arrow::field("rectification_parameters", arrow::fixed_size_list(arrow::float64(), 9)),
        arrow::field("projection_parameters", arrow::fixed_size_list(arrow::float64(), 12)),
        arrow::field("binning", vector2dStructType(), /*nullable=*/true),
        arrow::field("roi", roiType(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status CameraInfoBuilder::appendRow(const FieldMap& fields) {
    // height
    auto* hb = batch_builder_->GetFieldAs<arrow::UInt32Builder>(2);
    auto height = getUint32(fields, "height");
    if (height) ARROW_RETURN_NOT_OK(hb->Append(*height)); else ARROW_RETURN_NOT_OK(hb->Append(0));

    // width
    auto* wb = batch_builder_->GetFieldAs<arrow::UInt32Builder>(3);
    auto width = getUint32(fields, "width");
    if (width) ARROW_RETURN_NOT_OK(wb->Append(*width)); else ARROW_RETURN_NOT_OK(wb->Append(0));

    // distortion_model
    auto* dm = batch_builder_->GetFieldAs<arrow::StringBuilder>(4);
    auto model = getString(fields, "distortion_model");
    ARROW_RETURN_NOT_OK(dm->Append(model.value_or("")));

    // distortion_parameters (variable-length list)
    auto* dp = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(5));
    const auto* d_arr = getDoubleArray(fields, "d");
    if (!d_arr) d_arr = getDoubleArray(fields, "D");
    ARROW_RETURN_NOT_OK(dp->Append());
    auto* dp_val = static_cast<arrow::DoubleBuilder*>(dp->value_builder());
    if (d_arr) ARROW_RETURN_NOT_OK(dp_val->AppendValues(d_arr->data(), static_cast<int64_t>(d_arr->size())));

    // intrinsic_parameters (fixed_size_list 9)
    auto* ip = static_cast<arrow::FixedSizeListBuilder*>(batch_builder_->GetField(6));
    const auto* k_arr = getDoubleArray(fields, "k");
    if (!k_arr) k_arr = getDoubleArray(fields, "K");
    ARROW_RETURN_NOT_OK(ip->Append());
    auto* ip_val = static_cast<arrow::DoubleBuilder*>(ip->value_builder());
    if (k_arr && k_arr->size() == 9) {
        ARROW_RETURN_NOT_OK(ip_val->AppendValues(k_arr->data(), 9));
    } else {
        for (int i = 0; i < 9; ++i) ARROW_RETURN_NOT_OK(ip_val->Append(0.0));
    }

    // rectification_parameters (fixed_size_list 9)
    auto* rp = static_cast<arrow::FixedSizeListBuilder*>(batch_builder_->GetField(7));
    const auto* r_arr = getDoubleArray(fields, "r");
    if (!r_arr) r_arr = getDoubleArray(fields, "R");
    ARROW_RETURN_NOT_OK(rp->Append());
    auto* rp_val = static_cast<arrow::DoubleBuilder*>(rp->value_builder());
    if (r_arr && r_arr->size() == 9) {
        ARROW_RETURN_NOT_OK(rp_val->AppendValues(r_arr->data(), 9));
    } else {
        for (int i = 0; i < 9; ++i) ARROW_RETURN_NOT_OK(rp_val->Append(0.0));
    }

    // projection_parameters (fixed_size_list 12)
    auto* pp = static_cast<arrow::FixedSizeListBuilder*>(batch_builder_->GetField(8));
    const auto* p_arr = getDoubleArray(fields, "p");
    if (!p_arr) p_arr = getDoubleArray(fields, "P");
    ARROW_RETURN_NOT_OK(pp->Append());
    auto* pp_val = static_cast<arrow::DoubleBuilder*>(pp->value_builder());
    if (p_arr && p_arr->size() == 12) {
        ARROW_RETURN_NOT_OK(pp_val->AppendValues(p_arr->data(), 12));
    } else {
        for (int i = 0; i < 12; ++i) ARROW_RETURN_NOT_OK(pp_val->Append(0.0));
    }

    // binning: Vector2d struct {x, y}
    auto* bin_sb = structBuilder(9);
    auto bx = getNumericAsDouble(fields, "binning_x");
    auto by = getNumericAsDouble(fields, "binning_y");
    if (bx || by) {
        ARROW_RETURN_NOT_OK(bin_sb->Append());
        auto* bxb = static_cast<arrow::DoubleBuilder*>(bin_sb->field_builder(0));
        auto* byb = static_cast<arrow::DoubleBuilder*>(bin_sb->field_builder(1));
        if (bx) ARROW_RETURN_NOT_OK(bxb->Append(*bx)); else ARROW_RETURN_NOT_OK(bxb->AppendNull());
        if (by) ARROW_RETURN_NOT_OK(byb->Append(*by)); else ARROW_RETURN_NOT_OK(byb->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(bin_sb->AppendNull());
    }

    // roi: ROI struct {offset: {x, y}, height, width, do_rectify} + header
    auto* roi_sb = structBuilder(10);
    bool has_roi = fields.fields.count("roi.x_offset") || fields.fields.count("roi.height");
    if (has_roi) {
        ARROW_RETURN_NOT_OK(roi_sb->Append());
        // offset struct {x, y}
        auto* off_sb = static_cast<arrow::StructBuilder*>(roi_sb->field_builder(0));
        ARROW_RETURN_NOT_OK(off_sb->Append());
        auto* ox = static_cast<arrow::DoubleBuilder*>(off_sb->field_builder(0));
        auto* oy = static_cast<arrow::DoubleBuilder*>(off_sb->field_builder(1));
        auto xo = getNumericAsDouble(fields, "roi.x_offset");
        auto yo = getNumericAsDouble(fields, "roi.y_offset");
        if (xo) ARROW_RETURN_NOT_OK(ox->Append(*xo)); else ARROW_RETURN_NOT_OK(ox->Append(0.0));
        if (yo) ARROW_RETURN_NOT_OK(oy->Append(*yo)); else ARROW_RETURN_NOT_OK(oy->Append(0.0));

        // height, width
        auto* rh = static_cast<arrow::UInt32Builder*>(roi_sb->field_builder(1));
        auto* rw = static_cast<arrow::UInt32Builder*>(roi_sb->field_builder(2));
        auto roi_h = getUint32(fields, "roi.height");
        auto roi_w = getUint32(fields, "roi.width");
        if (roi_h) ARROW_RETURN_NOT_OK(rh->Append(*roi_h)); else ARROW_RETURN_NOT_OK(rh->Append(0));
        if (roi_w) ARROW_RETURN_NOT_OK(rw->Append(*roi_w)); else ARROW_RETURN_NOT_OK(rw->Append(0));

        // do_rectify
        auto* dr = static_cast<arrow::BooleanBuilder*>(roi_sb->field_builder(3));
        auto do_rect = getBool(fields, "roi.do_rectify");
        if (do_rect) ARROW_RETURN_NOT_OK(dr->Append(*do_rect)); else ARROW_RETURN_NOT_OK(dr->AppendNull());

        // roi header
        auto* roi_hb = static_cast<arrow::StructBuilder*>(roi_sb->field_builder(4));
        ARROW_RETURN_NOT_OK(roi_hb->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(roi_sb->AppendNull());
    }

    // header
    auto* header_b = structBuilder(11);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// BatteryStateBuilder
// BatteryState: {voltage, temperature, current, charge, capacity, design_capacity,
//   percentage (all float32), power_supply_status/health/technology (uint8),
//   present (bool), location/serial_number (string),
//   cell_voltage/cell_temperature (list<float32>, nullable)} + header
// ============================================================
void BatteryStateBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("voltage", arrow::float32()),
        arrow::field("temperature", arrow::float32()),
        arrow::field("current", arrow::float32()),
        arrow::field("charge", arrow::float32()),
        arrow::field("capacity", arrow::float32()),
        arrow::field("design_capacity", arrow::float32()),
        arrow::field("percentage", arrow::float32()),
        arrow::field("power_supply_status", arrow::uint8()),
        arrow::field("power_supply_health", arrow::uint8()),
        arrow::field("power_supply_technology", arrow::uint8()),
        arrow::field("present", arrow::boolean()),
        arrow::field("location", arrow::utf8()),
        arrow::field("serial_number", arrow::utf8()),
        arrow::field("cell_voltage", arrow::list(arrow::float32()), /*nullable=*/true),
        arrow::field("cell_temperature", arrow::list(arrow::float32()), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status BatteryStateBuilder::appendRow(const FieldMap& fields) {
    int idx = 2;
    // float32 fields: voltage, temperature, current, charge, capacity, design_capacity, percentage
    for (const auto& key : {"voltage", "temperature", "current", "charge",
                             "capacity", "design_capacity", "percentage"}) {
        auto* fb = batch_builder_->GetFieldAs<arrow::FloatBuilder>(idx++);
        auto val = getNumericAsFloat(fields, key);
        if (val) ARROW_RETURN_NOT_OK(fb->Append(*val));
        else ARROW_RETURN_NOT_OK(fb->Append(0.0f));
    }

    // uint8 fields: power_supply_status, power_supply_health, power_supply_technology
    for (const auto& key : {"power_supply_status", "power_supply_health",
                             "power_supply_technology"}) {
        auto* ub = batch_builder_->GetFieldAs<arrow::UInt8Builder>(idx++);
        auto val = getUint8(fields, key);
        if (val) ARROW_RETURN_NOT_OK(ub->Append(*val));
        else ARROW_RETURN_NOT_OK(ub->Append(0));
    }

    // present
    auto* pb = batch_builder_->GetFieldAs<arrow::BooleanBuilder>(idx++);
    auto present = getBool(fields, "present");
    if (present) ARROW_RETURN_NOT_OK(pb->Append(*present));
    else ARROW_RETURN_NOT_OK(pb->Append(false));

    // location, serial_number
    auto* loc_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(idx++);
    auto loc = getString(fields, "location");
    ARROW_RETURN_NOT_OK(loc_b->Append(loc.value_or("")));

    auto* sn_b = batch_builder_->GetFieldAs<arrow::StringBuilder>(idx++);
    auto sn = getString(fields, "serial_number");
    ARROW_RETURN_NOT_OK(sn_b->Append(sn.value_or("")));

    // cell_voltage: list<float32>
    auto* cv_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(idx++));
    const auto* cv = getDoubleArray(fields, "cell_voltage");
    if (cv && !cv->empty()) {
        ARROW_RETURN_NOT_OK(cv_b->Append());
        auto* cv_val = static_cast<arrow::FloatBuilder*>(cv_b->value_builder());
        for (double v : *cv) ARROW_RETURN_NOT_OK(cv_val->Append(static_cast<float>(v)));
    } else {
        ARROW_RETURN_NOT_OK(cv_b->AppendNull());
    }

    // cell_temperature: list<float32>
    auto* ct_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(idx++));
    const auto* ct = getDoubleArray(fields, "cell_temperature");
    if (ct && !ct->empty()) {
        ARROW_RETURN_NOT_OK(ct_b->Append());
        auto* ct_val = static_cast<arrow::FloatBuilder*>(ct_b->value_builder());
        for (double v : *ct) ARROW_RETURN_NOT_OK(ct_val->Append(static_cast<float>(v)));
    } else {
        ARROW_RETURN_NOT_OK(ct_b->AppendNull());
    }

    // header
    auto* header_b = structBuilder(idx);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// RobotJointBuilder
// RobotJoint: {names: list<string>, positions: list<float64>,
//              velocities: list<float64>, efforts: list<float64>} + header
// ============================================================
void RobotJointBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("names", arrow::list(arrow::utf8())),
        arrow::field("positions", arrow::list(arrow::float64())),
        arrow::field("velocities", arrow::list(arrow::float64())),
        arrow::field("efforts", arrow::list(arrow::float64())),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status RobotJointBuilder::appendRow(const FieldMap& fields) {
    // names
    auto* names_b = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(2));
    const auto* names = getStringArray(fields, "name");
    ARROW_RETURN_NOT_OK(names_b->Append());
    auto* names_val = static_cast<arrow::StringBuilder*>(names_b->value_builder());
    if (names) {
        for (const auto& n : *names) ARROW_RETURN_NOT_OK(names_val->Append(n));
    }

    // positions, velocities, efforts
    int idx = 3;
    for (const auto& key : {"position", "velocity", "effort"}) {
        auto* lb = static_cast<arrow::ListBuilder*>(batch_builder_->GetField(idx++));
        const auto* arr = getDoubleArray(fields, key);
        ARROW_RETURN_NOT_OK(lb->Append());
        auto* val_b = static_cast<arrow::DoubleBuilder*>(lb->value_builder());
        if (arr) ARROW_RETURN_NOT_OK(val_b->AppendValues(arr->data(), static_cast<int64_t>(arr->size())));
    }

    auto* header_b = structBuilder(6);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// NmeaSentenceBuilder
// NMEASentence: {sentence: string} + header
// ============================================================
void NmeaSentenceBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("sentence", arrow::utf8()),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status NmeaSentenceBuilder::appendRow(const FieldMap& fields) {
    auto* sb = batch_builder_->GetFieldAs<arrow::StringBuilder>(2);
    auto sentence = getString(fields, "sentence");
    ARROW_RETURN_NOT_OK(sb->Append(sentence.value_or("")));

    auto* header_b = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

// ============================================================
// RoiBuilder
// ROI: {offset: Vector2d{x, y}, height: uint32, width: uint32, do_rectify: bool} + header
// ============================================================
void RoiBuilder::initSchema() {
    schema_ = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64()),
        arrow::field("recording_timestamp_ns", arrow::int64()),
        arrow::field("offset", vector2dStructType()),
        arrow::field("height", arrow::uint32()),
        arrow::field("width", arrow::uint32()),
        arrow::field("do_rectify", arrow::boolean(), /*nullable=*/true),
        arrow::field("header", headerType(), /*nullable=*/true),
    });
}

arrow::Status RoiBuilder::appendRow(const FieldMap& fields) {
    // offset struct {x, y} — from x_offset, y_offset
    auto* off_sb = structBuilder(2);
    auto xo = getNumericAsDouble(fields, "x_offset");
    auto yo = getNumericAsDouble(fields, "y_offset");
    ARROW_RETURN_NOT_OK(off_sb->Append());
    auto* ox = static_cast<arrow::DoubleBuilder*>(off_sb->field_builder(0));
    auto* oy = static_cast<arrow::DoubleBuilder*>(off_sb->field_builder(1));
    if (xo) ARROW_RETURN_NOT_OK(ox->Append(*xo)); else ARROW_RETURN_NOT_OK(ox->Append(0.0));
    if (yo) ARROW_RETURN_NOT_OK(oy->Append(*yo)); else ARROW_RETURN_NOT_OK(oy->Append(0.0));

    auto* hb = batch_builder_->GetFieldAs<arrow::UInt32Builder>(3);
    auto h = getUint32(fields, "height");
    if (h) ARROW_RETURN_NOT_OK(hb->Append(*h)); else ARROW_RETURN_NOT_OK(hb->Append(0));

    auto* wb = batch_builder_->GetFieldAs<arrow::UInt32Builder>(4);
    auto w = getUint32(fields, "width");
    if (w) ARROW_RETURN_NOT_OK(wb->Append(*w)); else ARROW_RETURN_NOT_OK(wb->Append(0));

    auto* dr = batch_builder_->GetFieldAs<arrow::BooleanBuilder>(5);
    auto do_rect = getBool(fields, "do_rectify");
    if (do_rect) ARROW_RETURN_NOT_OK(dr->Append(*do_rect)); else ARROW_RETURN_NOT_OK(dr->AppendNull());

    auto* header_b = structBuilder(6);
    ARROW_RETURN_NOT_OK(appendHeader(header_b, fields));

    return arrow::Status::OK();
}

}  // namespace mosaico
