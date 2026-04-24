#pragma once
// Sensor builders: Image, CompressedImage, CameraInfo, BatteryState,
//                  RobotJoint, NMEASentence, ROI
#include "ontology/builders/typed_builder.hpp"

namespace mosaico {

class ImageBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "image"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class CompressedImageBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "compressed_image"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class CameraInfoBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "camera_info"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class BatteryStateBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "battery_state"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class RobotJointBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "robot_joint"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class NmeaSentenceBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "nmea_sentence"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

class RoiBuilder : public TypedBuilder {
 public:
    std::string ontologyTag() const override { return "roi"; }
 protected:
    void initSchema() override;
    arrow::Status appendRow(const FieldMap& fields) override;
};

}  // namespace mosaico
