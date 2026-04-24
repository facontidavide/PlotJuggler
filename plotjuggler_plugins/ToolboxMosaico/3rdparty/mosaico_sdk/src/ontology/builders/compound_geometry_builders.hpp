#pragma once
// Compound geometry builders: Pose, Velocity, Acceleration, Transform, ForceTorque
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class PoseBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "pose"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class VelocityBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "velocity"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class AccelerationBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "acceleration"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class TransformBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "transform"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class ForceTorqueBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "force_torque"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
