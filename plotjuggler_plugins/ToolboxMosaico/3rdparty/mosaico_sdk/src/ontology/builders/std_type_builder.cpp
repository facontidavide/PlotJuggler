#include "ontology/builders/std_type_builder.hpp"

namespace mosaico {

// Specializations of appendRow for each Arrow type.

template <>
arrow::Status StdTypeBuilder<arrow::StringType>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::StringBuilder>(2);
    auto val = getString(fields, "data");
    ARROW_RETURN_NOT_OK(db->Append(val.value_or("")));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::BooleanType>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::BooleanBuilder>(2);
    auto val = getBool(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(false));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::Int8Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::Int8Builder>(2);
    auto val = getInt8(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::Int16Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::Int16Builder>(2);
    auto val = getInt16(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::Int32Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::Int32Builder>(2);
    auto val = getInt32(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::Int64Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::Int64Builder>(2);
    auto val = getInt64(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::UInt8Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::UInt8Builder>(2);
    auto val = getUint8(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::UInt16Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::UInt16Builder>(2);
    auto val = getUint16(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::UInt32Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::UInt32Builder>(2);
    auto val = getUint32(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::UInt64Type>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::UInt64Builder>(2);
    auto val = getUint64(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::FloatType>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::FloatBuilder>(2);
    auto val = getFloat(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0.0f));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

template <>
arrow::Status StdTypeBuilder<arrow::DoubleType>::appendRow(const FieldMap& fields) {
    auto* db = batch_builder_->GetFieldAs<arrow::DoubleBuilder>(2);
    auto val = getDouble(fields, "data");
    if (val) ARROW_RETURN_NOT_OK(db->Append(*val));
    else ARROW_RETURN_NOT_OK(db->Append(0.0));
    auto* hb = structBuilder(3);
    ARROW_RETURN_NOT_OK(appendHeader(hb, fields));
    return arrow::Status::OK();
}

// Explicit instantiations
template class StdTypeBuilder<arrow::StringType>;
template class StdTypeBuilder<arrow::BooleanType>;
template class StdTypeBuilder<arrow::Int8Type>;
template class StdTypeBuilder<arrow::Int16Type>;
template class StdTypeBuilder<arrow::Int32Type>;
template class StdTypeBuilder<arrow::Int64Type>;
template class StdTypeBuilder<arrow::UInt8Type>;
template class StdTypeBuilder<arrow::UInt16Type>;
template class StdTypeBuilder<arrow::UInt32Type>;
template class StdTypeBuilder<arrow::UInt64Type>;
template class StdTypeBuilder<arrow::FloatType>;
template class StdTypeBuilder<arrow::DoubleType>;

}  // namespace mosaico
