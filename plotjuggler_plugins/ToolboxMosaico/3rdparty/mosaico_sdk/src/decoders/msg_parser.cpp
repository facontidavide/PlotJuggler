// tools/decoders/msg_parser.cpp
#include "decoders/msg_parser.hpp"

#include <arrow/api.h>

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mosaico {
namespace {

// ---------------------------------------------------------------------------
// Primitive type name -> PrimitiveType mapping
// ---------------------------------------------------------------------------
std::optional<PrimitiveType> parsePrimitive(const std::string& type_name) {
  static const std::unordered_map<std::string, PrimitiveType> kMap = {
      {"bool", PrimitiveType::kBool},
      {"int8", PrimitiveType::kInt8},
      {"uint8", PrimitiveType::kUint8},
      {"byte", PrimitiveType::kUint8},
      {"char", PrimitiveType::kUint8},
      {"int16", PrimitiveType::kInt16},
      {"uint16", PrimitiveType::kUint16},
      {"int32", PrimitiveType::kInt32},
      {"uint32", PrimitiveType::kUint32},
      {"int64", PrimitiveType::kInt64},
      {"uint64", PrimitiveType::kUint64},
      {"float32", PrimitiveType::kFloat32},
      {"float64", PrimitiveType::kFloat64},
      {"string", PrimitiveType::kString},
      {"wstring", PrimitiveType::kWstring},
  };
  auto it = kMap.find(type_name);
  if (it != kMap.end()) return it->second;
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// Check if a line is a separator (3+ consecutive '=' characters, nothing else)
// ---------------------------------------------------------------------------
bool isSeparator(const std::string& line) {
  if (line.size() < 3) return false;
  return line.find_first_not_of('=') == std::string::npos;
}

// ---------------------------------------------------------------------------
// Trim leading and trailing whitespace
// ---------------------------------------------------------------------------
std::string trim(const std::string& s) {
  auto start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  auto end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

// ---------------------------------------------------------------------------
// Split text into lines
// ---------------------------------------------------------------------------
std::vector<std::string> splitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    // Remove trailing \r if present (Windows line endings)
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(line);
  }
  return lines;
}

// ---------------------------------------------------------------------------
// Split text into blocks separated by lines of ='s
// ---------------------------------------------------------------------------
std::vector<std::vector<std::string>> splitBlocks(
    const std::vector<std::string>& lines) {
  std::vector<std::vector<std::string>> blocks;
  std::vector<std::string> current;
  for (const auto& line : lines) {
    if (isSeparator(trim(line))) {
      blocks.push_back(std::move(current));
      current.clear();
    } else {
      current.push_back(line);
    }
  }
  // Don't forget the last block
  blocks.push_back(std::move(current));
  return blocks;
}

// ---------------------------------------------------------------------------
// Parse a type string, extracting base type and array info.
// Examples: "float64" -> ("float64", 0)
//           "float64[9]" -> ("float64", 9)
//           "uint8[]" -> ("uint8", -1)
// ---------------------------------------------------------------------------
struct ParsedType {
  std::string base_type;
  int32_t array_size = 0;  // 0=scalar, >0=fixed, -1=dynamic
};

ParsedType parseType(const std::string& type_str) {
  ParsedType result;
  auto bracket_pos = type_str.find('[');
  if (bracket_pos == std::string::npos) {
    result.base_type = type_str;
    result.array_size = 0;
  } else {
    result.base_type = type_str.substr(0, bracket_pos);
    auto close_pos = type_str.find(']', bracket_pos);
    if (close_pos == std::string::npos) {
      // Malformed — treat as scalar
      result.base_type = type_str;
      result.array_size = 0;
    } else {
      auto inner = type_str.substr(bracket_pos + 1, close_pos - bracket_pos - 1);
      if (inner.empty()) {
        result.array_size = -1;  // dynamic array
      } else {
        // std::stoi throws std::invalid_argument / std::out_of_range on
        // non-numeric or overflowing bracket contents. A crafted .msg line
        // like `float64[abc] foo` should not escape as an exception out of
        // the SDK — fall back to the same scalar treatment used above for
        // unterminated brackets.
        try {
          result.array_size = std::stoi(inner);
        } catch (const std::exception&) {
          result.base_type = type_str;
          result.array_size = 0;
        }
      }
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// Parse a single block of .msg lines into a MsgSchema (fields only, no
// nested resolution yet).
// ---------------------------------------------------------------------------
std::shared_ptr<MsgSchema> parseBlock(const std::string& name,
                                       const std::vector<std::string>& lines) {
  auto schema = std::make_shared<MsgSchema>();
  schema->full_name = name;

  for (const auto& raw_line : lines) {
    // Strip inline comments
    std::string line = raw_line;
    auto hash_pos = line.find('#');
    if (hash_pos != std::string::npos) {
      line = line.substr(0, hash_pos);
    }
    line = trim(line);
    if (line.empty()) continue;

    // Skip constant definitions (lines like "uint8 FOO=42")
    if (line.find('=') != std::string::npos) continue;

    // Parse "type name [default_value]"
    std::istringstream iss(line);
    std::string type_str;
    std::string field_name;
    iss >> type_str >> field_name;
    if (type_str.empty() || field_name.empty()) continue;

    MsgField field;
    field.name = field_name;

    // Check for default value (remaining tokens)
    std::string rest;
    if (std::getline(iss >> std::ws, rest)) {
      field.default_value = trim(rest);
    }

    // Parse type
    auto parsed = parseType(type_str);
    field.array_size = parsed.array_size;

    auto prim = parsePrimitive(parsed.base_type);
    if (prim.has_value()) {
      field.primitive = *prim;
    } else {
      field.nested_type = parsed.base_type;
    }

    schema->fields.push_back(std::move(field));
  }

  return schema;
}

// ---------------------------------------------------------------------------
// Extract short name from a fully qualified type name.
// "geometry_msgs/Vector3" -> "Vector3"
// "Vector3" -> "Vector3"
// ---------------------------------------------------------------------------
std::string shortName(const std::string& full_name) {
  auto slash = full_name.rfind('/');
  if (slash == std::string::npos) return full_name;
  return full_name.substr(slash + 1);
}

// ---------------------------------------------------------------------------
// Resolve nested types in a schema tree.
// A malformed (or deliberately crafted) .msg concatenation can declare
// mutually-recursive types — e.g. pkg/A contains pkg/B and pkg/B contains
// pkg/A — in which case a naïve recursive walk would descend forever and
// blow the stack. The `visited` set closes cycles: once a schema has been
// traversed, we still wire up the nested pointer but skip re-recursing
// into it. This is the same guard the Python SDK relies on implicitly via
// its memoized resolver.
// ---------------------------------------------------------------------------
void resolveNested(
    const std::shared_ptr<MsgSchema>& schema,
    const std::unordered_map<std::string, std::shared_ptr<MsgSchema>>& defs,
    std::unordered_set<const MsgSchema*>& visited) {
  if (!visited.insert(schema.get()).second) {
    return;  // already processed — cycle or diamond
  }

  for (auto& field : schema->fields) {
    if (field.nested_type.empty()) continue;

    // Try exact match first
    auto it = defs.find(field.nested_type);
    if (it != defs.end()) {
      schema->nested[field.nested_type] = it->second;
      resolveNested(it->second, defs, visited);
      continue;
    }

    // Try matching by short name
    auto short_n = shortName(field.nested_type);
    for (const auto& [def_name, def_schema] : defs) {
      if (shortName(def_name) == short_n) {
        schema->nested[field.nested_type] = def_schema;
        resolveNested(def_schema, defs, visited);
        break;
      }
    }
  }
}

void resolveNested(
    const std::shared_ptr<MsgSchema>& schema,
    const std::unordered_map<std::string, std::shared_ptr<MsgSchema>>& defs) {
  std::unordered_set<const MsgSchema*> visited;
  resolveNested(schema, defs, visited);
}

// ---------------------------------------------------------------------------
// Arrow type conversion helpers
// ---------------------------------------------------------------------------
std::shared_ptr<arrow::DataType> primitiveToArrow(PrimitiveType p) {
  switch (p) {
    case PrimitiveType::kBool:    return arrow::boolean();
    case PrimitiveType::kInt8:    return arrow::int8();
    case PrimitiveType::kUint8:   return arrow::uint8();
    case PrimitiveType::kInt16:   return arrow::int16();
    case PrimitiveType::kUint16:  return arrow::uint16();
    case PrimitiveType::kInt32:   return arrow::int32();
    case PrimitiveType::kUint32:  return arrow::uint32();
    case PrimitiveType::kInt64:   return arrow::int64();
    case PrimitiveType::kUint64:  return arrow::uint64();
    case PrimitiveType::kFloat32: return arrow::float32();
    case PrimitiveType::kFloat64: return arrow::float64();
    case PrimitiveType::kString:  return arrow::utf8();
    case PrimitiveType::kWstring: return arrow::utf8();
  }
  return arrow::utf8();  // unreachable
}

std::shared_ptr<arrow::DataType> fieldToArrow(const MsgField& field,
                                               const MsgSchema& schema);

std::shared_ptr<arrow::DataType> nestedToArrow(const std::string& type_name,
                                                const MsgSchema& schema) {
  auto it = schema.nested.find(type_name);
  if (it == schema.nested.end()) {
    // Unresolved nested type — fall back to binary
    return arrow::binary();
  }
  const auto& nested = *it->second;
  arrow::FieldVector fields;
  for (const auto& f : nested.fields) {
    fields.push_back(arrow::field(f.name, fieldToArrow(f, nested)));
  }
  return arrow::struct_(fields);
}

std::shared_ptr<arrow::DataType> fieldToArrow(const MsgField& field,
                                               const MsgSchema& schema) {
  // Determine the element type
  std::shared_ptr<arrow::DataType> elem_type;
  if (field.primitive.has_value()) {
    elem_type = primitiveToArrow(*field.primitive);
  } else {
    elem_type = nestedToArrow(field.nested_type, schema);
  }

  // Apply array wrapping
  if (field.array_size > 0) {
    return arrow::fixed_size_list(arrow::field("item", elem_type),
                                  field.array_size);
  }
  if (field.array_size < 0) {
    return arrow::list(arrow::field("item", elem_type));
  }
  return elem_type;
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::shared_ptr<MsgSchema> parseMsgDef(const std::string& schema_name,
                                        const std::string& text) {
  auto lines = splitLines(text);
  auto blocks = splitBlocks(lines);

  if (blocks.empty()) return nullptr;

  // Parse the main type (first block)
  auto main_schema = parseBlock(schema_name, blocks[0]);
  if (!main_schema) return nullptr;

  // Parse sub-definitions
  std::unordered_map<std::string, std::shared_ptr<MsgSchema>> defs;
  for (size_t i = 1; i < blocks.size(); ++i) {
    const auto& block = blocks[i];
    if (block.empty()) continue;

    // First line should be "MSG: type_name"
    std::string first = trim(block[0]);
    if (first.substr(0, 5) != "MSG: ") continue;
    std::string type_name = trim(first.substr(5));

    // Parse remaining lines as the definition
    std::vector<std::string> def_lines(block.begin() + 1, block.end());
    auto def_schema = parseBlock(type_name, def_lines);
    if (def_schema) {
      defs[type_name] = def_schema;
    }
  }

  // Resolve nested types
  resolveNested(main_schema, defs);

  return main_schema;
}

std::shared_ptr<arrow::Schema> msgSchemaToArrow(const MsgSchema& schema) {
  arrow::FieldVector fields;

  // Prepend timestamp_ns
  fields.push_back(arrow::field("timestamp_ns", arrow::int64()));

  for (const auto& f : schema.fields) {
    fields.push_back(arrow::field(f.name, fieldToArrow(f, schema)));
  }

  return arrow::schema(fields);
}

}  // namespace mosaico
