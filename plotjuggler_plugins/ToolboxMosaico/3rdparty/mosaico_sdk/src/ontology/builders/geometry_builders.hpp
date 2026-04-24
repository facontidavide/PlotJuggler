#pragma once
// Geometry primitive builders: Vector3d, Point3d, Quaternion
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class Vector3dBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "vector3d"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class Point3dBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "point3d"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class QuaternionBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "quaternion"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
