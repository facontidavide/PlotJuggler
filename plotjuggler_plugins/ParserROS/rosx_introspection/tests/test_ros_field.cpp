#include <gtest/gtest.h>

#include "rosx_introspection/ros_field.hpp"

using RosMsgParser::ROSField;

// Regression coverage for the field-definition regex in ros_field.cpp.
// Covers the four valid forms of the ROS 2 IDL array syntax
// (scalar / fixed `[N]` / unbounded `[]` / bounded `[<=N]`) plus one
// malformed case that must keep the pre-fix behaviour.

namespace {
void ExpectField(
    const std::string& definition, bool is_array, int array_size, bool is_bounded, int max_size,
    const std::string& expected_name) {
  const ROSField f(definition);
  EXPECT_EQ(f.isArray(), is_array) << "definition: " << definition;
  EXPECT_EQ(f.arraySize(), array_size) << "definition: " << definition;
  EXPECT_EQ(f.isUpperBound(), is_bounded) << "definition: " << definition;
  EXPECT_EQ(f.maxSize(), max_size) << "definition: " << definition;
  EXPECT_EQ(f.name(), expected_name) << "definition: " << definition;
}
}  // namespace

TEST(ROSField, Scalar) {
  ExpectField("int32 foo", false, 1, false, -1, "foo");
}

TEST(ROSField, UnboundedSequence) {
  ExpectField("int32[] foo", true, -1, false, -1, "foo");
}

TEST(ROSField, FixedArray) {
  ExpectField("int32[5] foo", true, 5, false, -1, "foo");
}

TEST(ROSField, BoundedSmall) {
  ExpectField("int32[<=1] a", true, -1, true, 1, "a");
}

TEST(ROSField, BoundedTypical) {
  ExpectField("int32[<=5] foo", true, -1, true, 5, "foo");
}

TEST(ROSField, BoundedMid) {
  ExpectField("int64[<=12] bar", true, -1, true, 12, "bar");
}

TEST(ROSField, BoundedLarge) {
  ExpectField("uint8[<=4096] payload", true, -1, true, 4096, "payload");
}

TEST(ROSField, QualifiedBounded) {
  ExpectField("geometry_msgs/Point[<=3] pts", true, -1, true, 3, "pts");
}

// `[<5]` is not valid ROS 2 IDL syntax (missing `=`). Behaviour before
// the fix: the bracket group is silently dropped and the field parses
// as a scalar. The fix must preserve that behaviour — we do not want
// to accept a half-typed bound as if it were well-formed.
TEST(ROSField, MalformedMissingEquals) {
  ExpectField("int32[<5] foo", false, 1, false, -1, "foo");
}
