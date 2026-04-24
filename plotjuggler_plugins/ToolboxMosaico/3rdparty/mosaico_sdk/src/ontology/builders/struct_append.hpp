#pragma once
// Helpers for appending nested struct values to Arrow struct builders.

#include "ontology/builders/builder_utils.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/field_map.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>

#include <optional>
#include <string>
#include <vector>

namespace mosaico {

// Append a header struct (nullable) from FieldMap using a key prefix like "header."
// When no ROS header fields exist, synthesizes a header from _log_time_ns
// to match the Python SDK's Message.model_post_init behavior.
inline arrow::Status appendHeader(arrow::StructBuilder* hb, const FieldMap& fm,
                                   const std::string& prefix = "header.") {
    // Check if any header fields exist in the decoded ROS message
    bool has_header = fm.fields.count(prefix + "frame_id") ||
                      fm.fields.count(prefix + "stamp.sec") ||
                      fm.fields.count(prefix + "stamp.nanosec");

    if (!has_header) {
        // No ROS header — synthesize from _log_time_ns (matches Python SDK behavior)
        auto log_time = getUint64(fm, "_log_time_ns");
        if (!log_time) {
            return hb->AppendNull();
        }
        ARROW_RETURN_NOT_OK(hb->Append());
        // seq = null
        auto* seq_b = static_cast<arrow::UInt32Builder*>(hb->field_builder(0));
        ARROW_RETURN_NOT_OK(seq_b->AppendNull());
        // stamp = {sec, nanosec} from _log_time_ns
        auto* stamp_b = static_cast<arrow::StructBuilder*>(hb->field_builder(1));
        ARROW_RETURN_NOT_OK(stamp_b->Append());
        auto* sec_b = static_cast<arrow::Int64Builder*>(stamp_b->field_builder(0));
        auto* nsec_b = static_cast<arrow::UInt32Builder*>(stamp_b->field_builder(1));
        int64_t ns = static_cast<int64_t>(*log_time);
        ARROW_RETURN_NOT_OK(sec_b->Append(ns / 1000000000LL));
        ARROW_RETURN_NOT_OK(nsec_b->Append(static_cast<uint32_t>(ns % 1000000000LL)));
        // frame_id = null
        auto* fid_b = static_cast<arrow::StringBuilder*>(hb->field_builder(2));
        ARROW_RETURN_NOT_OK(fid_b->AppendNull());
        return arrow::Status::OK();
    }

    ARROW_RETURN_NOT_OK(hb->Append());
    // seq
    auto seq = getUint32(fm, prefix + "seq");
    auto* seq_b = static_cast<arrow::UInt32Builder*>(hb->field_builder(0));
    if (seq) ARROW_RETURN_NOT_OK(seq_b->Append(*seq));
    else ARROW_RETURN_NOT_OK(seq_b->AppendNull());

    // stamp struct
    auto* stamp_b = static_cast<arrow::StructBuilder*>(hb->field_builder(1));
    auto sec = getInt32(fm, prefix + "stamp.sec");
    auto nsec = getUint32(fm, prefix + "stamp.nanosec");
    if (sec || nsec) {
        ARROW_RETURN_NOT_OK(stamp_b->Append());
        auto* sec_b = static_cast<arrow::Int64Builder*>(stamp_b->field_builder(0));
        auto* nsec_b = static_cast<arrow::UInt32Builder*>(stamp_b->field_builder(1));
        if (sec) ARROW_RETURN_NOT_OK(sec_b->Append(static_cast<int64_t>(*sec)));
        else ARROW_RETURN_NOT_OK(sec_b->AppendNull());
        if (nsec) ARROW_RETURN_NOT_OK(nsec_b->Append(*nsec));
        else ARROW_RETURN_NOT_OK(nsec_b->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(stamp_b->AppendNull());
    }

    // frame_id
    auto frame_id = getString(fm, prefix + "frame_id");
    auto* fid_b = static_cast<arrow::StringBuilder*>(hb->field_builder(2));
    if (frame_id) ARROW_RETURN_NOT_OK(fid_b->Append(*frame_id));
    else ARROW_RETURN_NOT_OK(fid_b->AppendNull());

    return arrow::Status::OK();
}

// Append a covariance list + covariance_type to nullable list<double> + int16 builders
inline arrow::Status appendCovariance(arrow::ListBuilder* cov_b,
                                       arrow::Int16Builder* cov_type_b,
                                       const FieldMap& fm,
                                       const std::string& cov_key,
                                       const std::string& cov_type_key = "") {
    const auto* cov = getDoubleArray(fm, cov_key);
    if (cov && isValidCovariance(*cov)) {
        ARROW_RETURN_NOT_OK(cov_b->Append());
        auto* val_b = static_cast<arrow::DoubleBuilder*>(cov_b->value_builder());
        ARROW_RETURN_NOT_OK(val_b->AppendValues(cov->data(), static_cast<int64_t>(cov->size())));
    } else {
        ARROW_RETURN_NOT_OK(cov_b->AppendNull());
    }

    if (!cov_type_key.empty()) {
        auto ct = getInt32(fm, cov_type_key);
        if (ct) ARROW_RETURN_NOT_OK(cov_type_b->Append(static_cast<int16_t>(*ct)));
        else ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());
    }
    return arrow::Status::OK();
}

// Append a Vector3d struct (with full public schema: x,y,z + header + cov + cov_type)
// prefix e.g. "linear_acceleration."
inline arrow::Status appendVector3d(arrow::StructBuilder* sb, const FieldMap& fm,
                                     const std::string& prefix,
                                     const std::string& cov_key = "",
                                     bool nullable = false) {
    auto x = getNumericAsDouble(fm, prefix + "x");
    auto y = getNumericAsDouble(fm, prefix + "y");
    auto z = getNumericAsDouble(fm, prefix + "z");

    if (nullable && !x && !y && !z) {
        return sb->AppendNull();
    }

    ARROW_RETURN_NOT_OK(sb->Append());

    // x, y, z
    auto* xb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(0));
    auto* yb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(1));
    auto* zb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(2));
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());

    // header (index 3) - for inner vectors in compound types, typically null
    auto* hb = static_cast<arrow::StructBuilder*>(sb->field_builder(3));
    ARROW_RETURN_NOT_OK(hb->AppendNull());

    // covariance (index 4) + covariance_type (index 5)
    auto* cov_b = static_cast<arrow::ListBuilder*>(sb->field_builder(4));
    auto* cov_type_b = static_cast<arrow::Int16Builder*>(sb->field_builder(5));
    if (!cov_key.empty()) {
        const auto* cov = getDoubleArray(fm, cov_key);
        if (cov && isValidCovariance(*cov)) {
            ARROW_RETURN_NOT_OK(cov_b->Append());
            auto* val_b = static_cast<arrow::DoubleBuilder*>(cov_b->value_builder());
            ARROW_RETURN_NOT_OK(val_b->AppendValues(cov->data(), static_cast<int64_t>(cov->size())));
        } else {
            ARROW_RETURN_NOT_OK(cov_b->AppendNull());
        }
    } else {
        ARROW_RETURN_NOT_OK(cov_b->AppendNull());
    }
    ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());

    return arrow::Status::OK();
}

// Append a Quaternion struct (x,y,z,w + header + cov + cov_type)
inline arrow::Status appendQuaternion(arrow::StructBuilder* sb, const FieldMap& fm,
                                       const std::string& prefix,
                                       const std::string& cov_key = "",
                                       bool nullable = false) {
    auto x = getNumericAsDouble(fm, prefix + "x");
    auto y = getNumericAsDouble(fm, prefix + "y");
    auto z = getNumericAsDouble(fm, prefix + "z");
    auto w = getNumericAsDouble(fm, prefix + "w");

    if (nullable && !x && !y && !z && !w) {
        return sb->AppendNull();
    }

    ARROW_RETURN_NOT_OK(sb->Append());

    auto* xb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(0));
    auto* yb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(1));
    auto* zb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(2));
    auto* wb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(3));
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());
    if (w) ARROW_RETURN_NOT_OK(wb->Append(*w)); else ARROW_RETURN_NOT_OK(wb->AppendNull());

    // header (index 4)
    auto* hb = static_cast<arrow::StructBuilder*>(sb->field_builder(4));
    ARROW_RETURN_NOT_OK(hb->AppendNull());

    // covariance (index 5) + covariance_type (index 6)
    auto* cov_b = static_cast<arrow::ListBuilder*>(sb->field_builder(5));
    auto* cov_type_b = static_cast<arrow::Int16Builder*>(sb->field_builder(6));
    if (!cov_key.empty()) {
        const auto* cov = getDoubleArray(fm, cov_key);
        if (cov && isValidCovariance(*cov)) {
            ARROW_RETURN_NOT_OK(cov_b->Append());
            auto* val_b = static_cast<arrow::DoubleBuilder*>(cov_b->value_builder());
            ARROW_RETURN_NOT_OK(val_b->AppendValues(cov->data(), static_cast<int64_t>(cov->size())));
        } else {
            ARROW_RETURN_NOT_OK(cov_b->AppendNull());
        }
    } else {
        ARROW_RETURN_NOT_OK(cov_b->AppendNull());
    }
    ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());

    return arrow::Status::OK();
}

// Append a Point3d struct — same schema as Vector3d
inline arrow::Status appendPoint3d(arrow::StructBuilder* sb, const FieldMap& fm,
                                    const std::string& prefix,
                                    const std::string& cov_key = "",
                                    const std::string& cov_type_key = "",
                                    bool nullable = false) {
    auto x = getNumericAsDouble(fm, prefix + "x");
    auto y = getNumericAsDouble(fm, prefix + "y");
    auto z = getNumericAsDouble(fm, prefix + "z");

    if (nullable && !x && !y && !z) {
        return sb->AppendNull();
    }

    ARROW_RETURN_NOT_OK(sb->Append());

    auto* xb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(0));
    auto* yb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(1));
    auto* zb = static_cast<arrow::DoubleBuilder*>(sb->field_builder(2));
    if (x) ARROW_RETURN_NOT_OK(xb->Append(*x)); else ARROW_RETURN_NOT_OK(xb->AppendNull());
    if (y) ARROW_RETURN_NOT_OK(yb->Append(*y)); else ARROW_RETURN_NOT_OK(yb->AppendNull());
    if (z) ARROW_RETURN_NOT_OK(zb->Append(*z)); else ARROW_RETURN_NOT_OK(zb->AppendNull());

    // header (index 3)
    auto* hb = static_cast<arrow::StructBuilder*>(sb->field_builder(3));
    ARROW_RETURN_NOT_OK(hb->AppendNull());

    // covariance (index 4)
    auto* cov_b = static_cast<arrow::ListBuilder*>(sb->field_builder(4));
    auto* cov_type_b = static_cast<arrow::Int16Builder*>(sb->field_builder(5));
    if (!cov_key.empty()) {
        const auto* cov = getDoubleArray(fm, cov_key);
        if (cov && isValidCovariance(*cov)) {
            ARROW_RETURN_NOT_OK(cov_b->Append());
            auto* val_b = static_cast<arrow::DoubleBuilder*>(cov_b->value_builder());
            ARROW_RETURN_NOT_OK(val_b->AppendValues(cov->data(), static_cast<int64_t>(cov->size())));
        } else {
            ARROW_RETURN_NOT_OK(cov_b->AppendNull());
        }
    } else {
        ARROW_RETURN_NOT_OK(cov_b->AppendNull());
    }
    if (!cov_type_key.empty()) {
        auto ct = getInt32(fm, cov_type_key);
        if (ct) ARROW_RETURN_NOT_OK(cov_type_b->Append(static_cast<int16_t>(*ct)));
        else ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());
    } else {
        ARROW_RETURN_NOT_OK(cov_type_b->AppendNull());
    }
    return arrow::Status::OK();
}

}  // namespace mosaico
