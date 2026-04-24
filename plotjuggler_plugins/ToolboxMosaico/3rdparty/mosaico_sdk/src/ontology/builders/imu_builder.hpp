#pragma once
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class ImuBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "imu"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
