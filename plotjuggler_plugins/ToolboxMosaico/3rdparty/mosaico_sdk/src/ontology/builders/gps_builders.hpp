#pragma once
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class GpsBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "gps"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class GpsStatusBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "gps_status"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
