#pragma once
// StdTypeBuilder<ArrowType> — template for std_msgs primitives.

#include "ontology/builders/typed_builder.hpp"
#include "ontology/builders/schema_types.hpp"
#include "ontology/builders/struct_append.hpp"

#include <arrow/api.h>

#include <string>

namespace mosaico {

template <typename ArrowType>
class StdTypeBuilder : public TypedBuilder {
 public:
    explicit StdTypeBuilder(std::string tag) : tag_(std::move(tag)) {}
    std::string ontologyTag() const override { return tag_; }

 protected:
    void initSchema() override {
        schema_ = arrow::schema({
            arrow::field("timestamp_ns", arrow::int64()),
            arrow::field("recording_timestamp_ns", arrow::int64()),
            arrow::field("data", std::make_shared<ArrowType>()),
            arrow::field("header", headerType(), /*nullable=*/true),
        });
    }

    arrow::Status appendRow(const FieldMap& fields) override;

 private:
    std::string tag_;
};

// --- Template specialization declarations ---
template <> arrow::Status StdTypeBuilder<arrow::StringType>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::BooleanType>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::Int8Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::Int16Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::Int32Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::Int64Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::UInt8Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::UInt16Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::UInt32Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::UInt64Type>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::FloatType>::appendRow(const FieldMap& fields);
template <> arrow::Status StdTypeBuilder<arrow::DoubleType>::appendRow(const FieldMap& fields);

}  // namespace mosaico
