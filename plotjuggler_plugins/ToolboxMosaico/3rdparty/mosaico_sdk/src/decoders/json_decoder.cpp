// tools/decoders/json_decoder.cpp
#include "decoders/json_decoder.hpp"

#include <nlohmann/json.hpp>

#include <string_view>
#include <vector>

using json = nlohmann::json;

namespace mosaico {
namespace {

// Recursively flatten a JSON object into dot-path keys in the FieldMap.
void flattenJson(const json& obj, const std::string& prefix, FieldMap& out) {
  for (auto it = obj.begin(); it != obj.end(); ++it) {
    std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
    const auto& val = it.value();

    if (val.is_object()) {
      flattenJson(val, key, out);
    } else if (val.is_array()) {
      // Check what's inside the array
      if (val.empty()) {
        out.fields[key] = std::vector<double>{};
      } else if (val[0].is_number()) {
        std::vector<double> nums;
        nums.reserve(val.size());
        for (const auto& elem : val) {
          nums.push_back(elem.get<double>());
        }
        out.fields[key] = std::move(nums);
      } else if (val[0].is_string()) {
        std::vector<std::string> strs;
        strs.reserve(val.size());
        for (const auto& elem : val) {
          strs.push_back(elem.get<std::string>());
        }
        out.fields[key] = std::move(strs);
      } else {
        // Mixed or nested arrays: store as JSON string
        out.fields[key] = val.dump();
      }
    } else if (val.is_number_float()) {
      out.fields[key] = val.get<double>();
    } else if (val.is_number_integer()) {
      if (val.is_number_unsigned()) {
        out.fields[key] = static_cast<int64_t>(val.get<uint64_t>());
      } else {
        out.fields[key] = val.get<int64_t>();
      }
    } else if (val.is_boolean()) {
      out.fields[key] = val.get<bool>();
    } else if (val.is_string()) {
      out.fields[key] = val.get<std::string>();
    }
    // null values are simply omitted
  }
}

}  // namespace

std::vector<std::string> JsonDecoder::supportedEncodings() const {
  return {"json"};
}

arrow::Status JsonDecoder::decode(
    const std::byte* data, uint64_t size,
    const DecoderContext& /*ctx*/,
    FieldMap& out) {
  std::string_view payload(reinterpret_cast<const char*>(data), size);
  json obj;
  try {
    obj = json::parse(payload);
  } catch (...) {
    return arrow::Status::Invalid("Failed to parse JSON message");
  }
  if (!obj.is_object()) {
    return arrow::Status::Invalid("JSON message is not an object");
  }

  // flattenJson classifies arrays by peeking at the first element, then
  // calls .get<T>() on every element unguarded. Any mixed-type or
  // null-containing array ([1, 2, "three"], [1, null, 3]) throws a
  // json::type_error mid-iteration. Catch it here — a single malformed
  // message must not abort the entire multi-channel MCAP upload.
  try {
    flattenJson(obj, "", out);
  } catch (const json::exception& e) {
    return arrow::Status::Invalid("Failed to flatten JSON message: ",
                                  e.what());
  }
  return arrow::Status::OK();
}

}  // namespace mosaico
