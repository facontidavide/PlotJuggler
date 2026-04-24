#pragma once

#include <cstdint>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mosaico {

struct FieldMap;

using FieldValue = std::variant<
    bool,
    int8_t, int16_t, int32_t, int64_t,
    uint8_t, uint16_t, uint32_t, uint64_t,
    float, double,
    std::string,
    std::vector<uint8_t>,
    std::vector<double>,
    std::vector<std::string>,
    std::vector<FieldMap>
>;

struct FieldMap {
    std::unordered_map<std::string, FieldValue> fields;
};

// Find which prefix, from the given candidates (tried in order),
// produces a key that exists in the FieldMap when prepended to leaf_key.
// Returns the matching prefix, or the last prefix if none match.
inline std::string resolvePrefix(const FieldMap& fm,
                                  const std::string& leaf_key,
                                  std::initializer_list<std::string> prefixes) {
    std::string last;
    for (const auto& p : prefixes) {
        last = p;
        if (fm.fields.count(p + leaf_key)) return p;
    }
    return last;
}

}  // namespace mosaico
