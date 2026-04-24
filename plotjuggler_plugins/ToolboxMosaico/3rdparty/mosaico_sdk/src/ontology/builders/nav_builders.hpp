#pragma once
// Navigation builders: MotionState (Odometry), FrameTransform (TFMessage)
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class MotionStateBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "motion_state"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

// FrameTransformBuilder is special: one ROS message produces multiple rows
// (one per transform in the "transforms" array).
class FrameTransformBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "frame_transform"; }

    // Override append to handle multi-row expansion
    arrow::Status append(const FieldMap& fields) override;

 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
