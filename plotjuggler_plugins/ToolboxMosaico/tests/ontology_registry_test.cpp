#include "ontology/ontology_registry.hpp"

#include <arrow/type.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace mosaico {

// The full set of tags that must match the Python SDK's Serializable subclass
// registry. Adding or removing a tag here is a wire-contract change.
static const std::vector<std::string> kExpectedTags = {
    // Geometry primitives
    "vector3d", "point3d", "quaternion",
    // Compound geometry
    "pose", "velocity", "acceleration", "transform", "force_torque",
    // IMU / GPS
    "imu", "gps", "gps_status",
    // Navigation
    "motion_state", "frame_transform",
    // Sensors
    "image", "compressed_image", "camera_info", "battery_state",
    "robot_joint", "nmea_sentence", "roi",
    // Std types
    "string", "boolean",
    "integer8", "integer16", "integer32", "integer64",
    "unsigned8", "unsigned16", "unsigned32", "unsigned64",
    "floating32", "floating64",
};

TEST(OntologyRegistry, FindsAllExpectedTags)
{
    OntologyRegistry registry;
    for (const auto& tag : kExpectedTags) {
        EXPECT_NE(registry.find(tag), nullptr) << "missing tag: " << tag;
    }
}

TEST(OntologyRegistry, ReturnsNullForUnknownTag)
{
    OntologyRegistry registry;
    EXPECT_EQ(registry.find("not_a_real_tag"), nullptr);
    EXPECT_EQ(registry.find(""), nullptr);
    EXPECT_EQ(registry.find("raw"), nullptr);
}

// Catches registration mismatches (e.g. registering an ImuBuilder under the
// "gps" key). Each builder's ontologyTag() must equal the key used to look
// it up.
TEST(OntologyRegistry, BuilderTagMatchesRegistryKey)
{
    OntologyRegistry registry;
    for (const auto& tag : kExpectedTags) {
        auto* builder = registry.find(tag);
        ASSERT_NE(builder, nullptr) << "missing tag: " << tag;
        EXPECT_EQ(builder->ontologyTag(), tag);
    }
}

// Each builder must produce a non-null Arrow schema with at least the
// timestamp columns that every topic shares.
TEST(OntologyRegistry, BuildersExposeSchemaWithTimestampColumns)
{
    OntologyRegistry registry;
    for (const auto& tag : kExpectedTags) {
        auto* builder = registry.find(tag);
        ASSERT_NE(builder, nullptr) << "missing tag: " << tag;
        auto schema = builder->schema();
        ASSERT_NE(schema, nullptr) << "null schema for tag: " << tag;
        EXPECT_TRUE(schema->GetFieldByName("timestamp_ns") != nullptr)
            << "tag '" << tag << "' missing timestamp_ns";
        EXPECT_TRUE(schema->GetFieldByName("recording_timestamp_ns") != nullptr)
            << "tag '" << tag << "' missing recording_timestamp_ns";
    }
}

// Spot-check a couple of schemas where the shape is part of the wire contract
// and not derivable from "has timestamp_ns".
TEST(OntologyRegistry, ImuSchemaHasExpectedStructFields)
{
    OntologyRegistry registry;
    auto* builder = registry.find("imu");
    ASSERT_NE(builder, nullptr);
    auto schema = builder->schema();
    ASSERT_NE(schema, nullptr);
    EXPECT_TRUE(schema->GetFieldByName("acceleration") != nullptr);
    EXPECT_TRUE(schema->GetFieldByName("angular_velocity") != nullptr);
    EXPECT_TRUE(schema->GetFieldByName("orientation") != nullptr);
}

TEST(OntologyRegistry, Vector3dSchemaIsFlatXYZ)
{
    OntologyRegistry registry;
    auto* builder = registry.find("vector3d");
    ASSERT_NE(builder, nullptr);
    auto schema = builder->schema();
    ASSERT_NE(schema, nullptr);
    auto x = schema->GetFieldByName("x");
    auto y = schema->GetFieldByName("y");
    auto z = schema->GetFieldByName("z");
    ASSERT_TRUE(x != nullptr);
    ASSERT_TRUE(y != nullptr);
    ASSERT_TRUE(z != nullptr);
    EXPECT_EQ(x->type()->id(), arrow::Type::DOUBLE);
    EXPECT_EQ(y->type()->id(), arrow::Type::DOUBLE);
    EXPECT_EQ(z->type()->id(), arrow::Type::DOUBLE);
}

TEST(OntologyRegistry, StdTypeBuilderDataColumnMatchesTagType)
{
    OntologyRegistry registry;

    struct Case { const char* tag; arrow::Type::type expected; };
    const Case cases[] = {
        {"string",     arrow::Type::STRING},
        {"boolean",    arrow::Type::BOOL},
        {"integer8",   arrow::Type::INT8},
        {"integer16",  arrow::Type::INT16},
        {"integer32",  arrow::Type::INT32},
        {"integer64",  arrow::Type::INT64},
        {"unsigned8",  arrow::Type::UINT8},
        {"unsigned16", arrow::Type::UINT16},
        {"unsigned32", arrow::Type::UINT32},
        {"unsigned64", arrow::Type::UINT64},
        {"floating32", arrow::Type::FLOAT},
        {"floating64", arrow::Type::DOUBLE},
    };
    for (const auto& c : cases) {
        auto* builder = registry.find(c.tag);
        ASSERT_NE(builder, nullptr) << c.tag;
        auto data = builder->schema()->GetFieldByName("data");
        ASSERT_TRUE(data != nullptr) << c.tag;
        EXPECT_EQ(data->type()->id(), c.expected) << c.tag;
    }
}

}  // namespace mosaico
