#pragma once

#include "ontology/field_map.hpp"

#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type_fwd.h>

#include <memory>
#include <string>

namespace mosaico {

class OntologyBuilder {
 public:
    virtual ~OntologyBuilder() = default;
    virtual std::string ontologyTag() const = 0;
    virtual std::shared_ptr<arrow::Schema> schema() const = 0;
    virtual arrow::Status append(const FieldMap& fields) = 0;
    virtual arrow::Result<std::shared_ptr<arrow::RecordBatch>> flush() = 0;
    virtual bool shouldFlush() const = 0;
};

}  // namespace mosaico
