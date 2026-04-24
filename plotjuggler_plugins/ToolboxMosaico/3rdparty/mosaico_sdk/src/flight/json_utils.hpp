// src/flight/json_utils.hpp — small helpers for reading nlohmann::json.
//
// The goal is to split JSON extraction into discrete, named steps instead
// of chaining `contains()`, type checks, indexing, and `.get<T>()` on one
// line. Each helper returns an `std::optional` so the caller can see,
// step by step:
//   - did the key exist?
//   - was the value the expected type?
//   - what was the value?
//
// This makes the code easier to read and dramatically easier to debug:
// every meaningful intermediate has a name, so you can place a breakpoint
// after any step and inspect exactly what the server sent.
#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace mosaico {

using json = nlohmann::json;

// Returns a pointer to parent[key] if it exists and is an object,
// otherwise nullptr. Callers check the pointer before dereferencing.
inline const json* tryGetObject(const json& parent, const std::string& key)
{
    if (!parent.is_object()) return nullptr;
    if (!parent.contains(key)) return nullptr;
    const auto& node = parent[key];
    if (!node.is_object()) return nullptr;
    return &node;
}

// Returns a pointer to parent[key] if it exists and is an array,
// otherwise nullptr.
inline const json* tryGetArray(const json& parent, const std::string& key)
{
    if (!parent.is_object()) return nullptr;
    if (!parent.contains(key)) return nullptr;
    const auto& node = parent[key];
    if (!node.is_array()) return nullptr;
    return &node;
}

inline std::optional<std::string> tryGetString(const json& parent,
                                               const std::string& key)
{
    if (!parent.is_object()) return std::nullopt;
    if (!parent.contains(key)) return std::nullopt;
    const auto& node = parent[key];
    if (node.is_null()) return std::nullopt;
    if (!node.is_string()) return std::nullopt;
    return node.get<std::string>();
}

inline std::optional<int64_t> tryGetInt64(const json& parent,
                                          const std::string& key)
{
    if (!parent.is_object()) return std::nullopt;
    if (!parent.contains(key)) return std::nullopt;
    const auto& node = parent[key];
    if (node.is_null()) return std::nullopt;
    if (!node.is_number_integer() && !node.is_number_unsigned())
    {
        return std::nullopt;
    }
    return node.get<int64_t>();
}

inline std::optional<uint64_t> tryGetUint64(const json& parent,
                                            const std::string& key)
{
    if (!parent.is_object()) return std::nullopt;
    if (!parent.contains(key)) return std::nullopt;
    const auto& node = parent[key];
    if (node.is_null()) return std::nullopt;
    if (!node.is_number_unsigned() && !node.is_number_integer())
    {
        return std::nullopt;
    }
    return node.get<uint64_t>();
}

inline std::optional<bool> tryGetBool(const json& parent,
                                      const std::string& key)
{
    if (!parent.is_object()) return std::nullopt;
    if (!parent.contains(key)) return std::nullopt;
    const auto& node = parent[key];
    if (node.is_null()) return std::nullopt;
    if (!node.is_boolean()) return std::nullopt;
    return node.get<bool>();
}

// Many server responses wrap the payload as {"response": <payload>}.
// This unwraps one layer when present, otherwise returns the input as-is.
inline const json& unwrapResponse(const json& root)
{
    if (!root.is_object()) return root;
    if (!root.contains("response")) return root;
    return root["response"];
}

}  // namespace mosaico
