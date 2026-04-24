#include "ontology/builders/builder_utils.hpp"

#include <algorithm>
#include <cstdint>

namespace mosaico {

std::pair<int64_t, int64_t> extractTimestamps(const FieldMap& fm) {
    // recording_timestamp_ns is always from _log_time_ns
    int64_t recording_ts = 0;
    {
        auto it = fm.fields.find("_log_time_ns");
        if (it != fm.fields.end() && std::holds_alternative<uint64_t>(it->second)) {
            recording_ts = static_cast<int64_t>(std::get<uint64_t>(it->second));
        }
    }

    // timestamp_ns: prefer header.stamp.sec + header.stamp.nanosec
    auto sec_it = fm.fields.find("header.stamp.sec");
    auto nsec_it = fm.fields.find("header.stamp.nanosec");

    if (sec_it != fm.fields.end() && nsec_it != fm.fields.end() &&
        std::holds_alternative<int32_t>(sec_it->second) &&
        std::holds_alternative<uint32_t>(nsec_it->second)) {
        int64_t sec = std::get<int32_t>(sec_it->second);
        int64_t nsec = std::get<uint32_t>(nsec_it->second);
        int64_t ts = sec * 1'000'000'000LL + nsec;
        return {ts, recording_ts};
    }

    // Fall back to _log_time_ns for timestamp_ns as well
    return {recording_ts, recording_ts};
}

bool isValidCovariance(const std::vector<double>& cov) {
    if (cov.empty()) {
        return false;
    }
    if (cov[0] == -1.0) {
        return false;
    }
    bool all_zero = std::all_of(cov.begin(), cov.end(), [](double v) { return v == 0.0; });
    return !all_zero;
}

std::optional<double> getDouble(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<double>(it->second)) return std::nullopt;
    return std::get<double>(it->second);
}

std::optional<float> getFloat(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<float>(it->second)) return std::nullopt;
    return std::get<float>(it->second);
}

std::optional<int32_t> getInt32(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<int32_t>(it->second)) return std::nullopt;
    return std::get<int32_t>(it->second);
}

std::optional<uint32_t> getUint32(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<uint32_t>(it->second)) return std::nullopt;
    return std::get<uint32_t>(it->second);
}

std::optional<int64_t> getInt64(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<int64_t>(it->second)) return std::nullopt;
    return std::get<int64_t>(it->second);
}

std::optional<uint64_t> getUint64(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<uint64_t>(it->second)) return std::nullopt;
    return std::get<uint64_t>(it->second);
}

std::optional<std::string> getString(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<std::string>(it->second)) return std::nullopt;
    return std::get<std::string>(it->second);
}

std::optional<bool> getBool(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<bool>(it->second)) return std::nullopt;
    return std::get<bool>(it->second);
}

const std::vector<double>* getDoubleArray(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return nullptr;
    if (!std::holds_alternative<std::vector<double>>(it->second)) return nullptr;
    return &std::get<std::vector<double>>(it->second);
}

const std::vector<uint8_t>* getByteArray(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return nullptr;
    if (!std::holds_alternative<std::vector<uint8_t>>(it->second)) return nullptr;
    return &std::get<std::vector<uint8_t>>(it->second);
}

const std::vector<std::string>* getStringArray(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return nullptr;
    if (!std::holds_alternative<std::vector<std::string>>(it->second)) return nullptr;
    return &std::get<std::vector<std::string>>(it->second);
}

std::optional<int8_t> getInt8(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<int8_t>(it->second)) return std::nullopt;
    return std::get<int8_t>(it->second);
}

std::optional<uint8_t> getUint8(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<uint8_t>(it->second)) return std::nullopt;
    return std::get<uint8_t>(it->second);
}

std::optional<int16_t> getInt16(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<int16_t>(it->second)) return std::nullopt;
    return std::get<int16_t>(it->second);
}

std::optional<uint16_t> getUint16(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    if (!std::holds_alternative<uint16_t>(it->second)) return std::nullopt;
    return std::get<uint16_t>(it->second);
}

const std::vector<FieldMap>* getFieldMapArray(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return nullptr;
    if (!std::holds_alternative<std::vector<FieldMap>>(it->second)) return nullptr;
    return &std::get<std::vector<FieldMap>>(it->second);
}

std::optional<double> getNumericAsDouble(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    return std::visit(
        [](const auto& v) -> std::optional<double> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_arithmetic_v<T>) {
                return static_cast<double>(v);
            }
            return std::nullopt;
        },
        it->second);
}

std::optional<float> getNumericAsFloat(const FieldMap& fm, const std::string& key) {
    auto it = fm.fields.find(key);
    if (it == fm.fields.end()) return std::nullopt;
    return std::visit(
        [](const auto& v) -> std::optional<float> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_arithmetic_v<T>) {
                return static_cast<float>(v);
            }
            return std::nullopt;
        },
        it->second);
}

}  // namespace mosaico
