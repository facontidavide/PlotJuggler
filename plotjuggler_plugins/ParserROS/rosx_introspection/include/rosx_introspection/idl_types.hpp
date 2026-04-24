#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "rosx_introspection/ros_type.hpp"

namespace RosMsgParser {

struct EnumValue {
  std::string name;
  int32_t value = 0;
  std::optional<int32_t> dds_compat_value;

  int32_t ddsCompatValue() const {
    return dds_compat_value.value_or(value);
  }
};

struct EnumDefinition {
  ROSType id;
  std::vector<EnumValue> values;
};

/// Lightweight field description for union cases.
/// We avoid using ROSField here to prevent circular dependencies.
struct UnionCaseField {
  ROSType type;
  std::string field_name;
  bool is_array = false;
  int array_size = 1;  // 1=scalar, -1=sequence, >1=fixed array
};

struct DiscriminatedUnion {
  ROSType id;
  std::string discriminant_type;
  // Key: discriminant value as string (enum name or integer)
  std::unordered_map<std::string, UnionCaseField> cases;
  std::optional<UnionCaseField> default_case;
};

struct TypedefAlias {
  ROSType id;
  std::string base_type;
  BuiltinType resolved_type = OTHER;
};

}  // namespace RosMsgParser
