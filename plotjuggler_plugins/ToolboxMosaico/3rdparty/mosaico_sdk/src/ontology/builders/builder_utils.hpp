#pragma once
#include "ontology/field_map.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mosaico {

// Extract timestamp_ns and recording_timestamp_ns from a FieldMap.
// timestamp_ns: from header.stamp.sec + header.stamp.nanosec (falls back to _log_time_ns)
// recording_timestamp_ns: always from _log_time_ns
std::pair<int64_t, int64_t> extractTimestamps(const FieldMap& fm);

// Check if a covariance array is valid (not all zeros, first element not -1).
bool isValidCovariance(const std::vector<double>& cov);

// Type-safe field accessors — return nullopt if key is missing or wrong type.
std::optional<double> getDouble(const FieldMap& fm, const std::string& key);
std::optional<float> getFloat(const FieldMap& fm, const std::string& key);
std::optional<int32_t> getInt32(const FieldMap& fm, const std::string& key);
std::optional<uint32_t> getUint32(const FieldMap& fm, const std::string& key);
std::optional<int64_t> getInt64(const FieldMap& fm, const std::string& key);
std::optional<uint64_t> getUint64(const FieldMap& fm, const std::string& key);
std::optional<std::string> getString(const FieldMap& fm, const std::string& key);
std::optional<bool> getBool(const FieldMap& fm, const std::string& key);
const std::vector<double>* getDoubleArray(const FieldMap& fm, const std::string& key);
const std::vector<uint8_t>* getByteArray(const FieldMap& fm, const std::string& key);
const std::vector<std::string>* getStringArray(const FieldMap& fm, const std::string& key);
std::optional<int8_t> getInt8(const FieldMap& fm, const std::string& key);
std::optional<uint8_t> getUint8(const FieldMap& fm, const std::string& key);
std::optional<int16_t> getInt16(const FieldMap& fm, const std::string& key);
std::optional<uint16_t> getUint16(const FieldMap& fm, const std::string& key);
const std::vector<FieldMap>* getFieldMapArray(const FieldMap& fm, const std::string& key);

// Numeric coercion: try to get a double from any numeric type
std::optional<double> getNumericAsDouble(const FieldMap& fm, const std::string& key);

// Numeric coercion: try to get a float from any numeric type
std::optional<float> getNumericAsFloat(const FieldMap& fm, const std::string& key);

}  // namespace mosaico
