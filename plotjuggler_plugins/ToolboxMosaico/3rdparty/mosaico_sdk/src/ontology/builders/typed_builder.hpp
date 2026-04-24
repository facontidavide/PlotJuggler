#pragma once
// Base template for ontology builders with a fixed schema.
// Subclasses only need to provide ontologyTag(), the schema (via initSchema),
// and appendRow() for the data-specific columns.

#include "ontology/ontology_builder.hpp"
#include "ontology/builders/builder_utils.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>
#include <arrow/record_batch.h>

#include <memory>
#include <string>

namespace mosaico {

class TypedBuilder : public OntologyBuilder {
 public:
    std::shared_ptr<arrow::Schema> schema() const override {
        ensureSchema();
        return schema_;
    }

    arrow::Status append(const FieldMap& fields) override {
        ensureSchema();
        if (!batch_builder_) {
            ARROW_ASSIGN_OR_RAISE(batch_builder_,
                arrow::RecordBatchBuilder::Make(schema_, arrow::default_memory_pool()));
        }
        auto [ts, rec_ts] = extractTimestamps(fields);
        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(0)->Append(ts));
        ARROW_RETURN_NOT_OK(batch_builder_->GetFieldAs<arrow::Int64Builder>(1)->Append(rec_ts));
        ARROW_RETURN_NOT_OK(appendRow(fields));
        ++row_count_;
        return arrow::Status::OK();
    }

    arrow::Result<std::shared_ptr<arrow::RecordBatch>> flush() override {
        // schema_ is populated eagerly by ensureSchema() (via schema()),
        // but batch_builder_ is only created lazily on the first append().
        // A channel with zero messages — or zero successfully-decoded
        // messages — reaches flush() with schema_ set and batch_builder_
        // still null; dereferencing it would SIGSEGV on the upload worker
        // thread. When there is nothing to flush, return an empty/null
        // batch rather than crashing.
        if (!batch_builder_) {
            return nullptr;
        }
        ARROW_ASSIGN_OR_RAISE(auto batch, batch_builder_->Flush());
        row_count_ = 0;
        return batch;
    }

    bool shouldFlush() const override { return row_count_ >= kBatchSize; }

 protected:
    static constexpr int64_t kBatchSize = 65536;

    // Subclass must set schema_ and call initSchema() to define fields.
    // The first two fields must always be timestamp_ns (int64) and recording_timestamp_ns (int64).
    mutable std::shared_ptr<arrow::Schema> schema_;
    std::unique_ptr<arrow::RecordBatchBuilder> batch_builder_;
    int64_t row_count_ = 0;

    virtual void initSchema() = 0;

    void ensureSchema() const {
        if (!schema_) {
            // initSchema() is non-const but only sets schema_, which is mutable.
            const_cast<TypedBuilder*>(this)->initSchema();
        }
    }

    // Append data columns starting from field index 2.
    virtual arrow::Status appendRow(const FieldMap& fields) = 0;

    // Helper: get a struct builder at column index i
    arrow::StructBuilder* structBuilder(int i) {
        return static_cast<arrow::StructBuilder*>(
            batch_builder_->GetField(i));
    }
};

}  // namespace mosaico
