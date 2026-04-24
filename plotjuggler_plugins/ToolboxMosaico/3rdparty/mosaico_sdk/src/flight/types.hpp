// src/flight/types.hpp
#pragma once

#include <arrow/type_fwd.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace mosaico {

struct SequenceInfo {
    std::string name;
    int64_t min_ts_ns = 0;
    int64_t max_ts_ns = 0;
    int64_t total_size_bytes = 0;
    std::unordered_map<std::string, std::string> user_metadata;
};

struct TopicInfo {
    std::string topic_name;
    std::string ontology_tag;
    std::shared_ptr<arrow::Schema> schema;
    std::unordered_map<std::string, std::string> user_metadata;
    std::string ticket_bytes;
    int64_t min_ts_ns = 0;
    int64_t max_ts_ns = 0;
};

struct TimeRange {
    std::optional<int64_t> start_ns;
    std::optional<int64_t> end_ns;
};

struct PullResult {
    std::shared_ptr<arrow::Schema> schema;
    std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
};

// Progress hook for streaming reads (both Flight pullTopic and MCAP reads).
//   rows         — cumulative rows read so far.
//   bytes        — cumulative uncompressed bytes read so far.
//   total_bytes  — total size the source advertised up front, or 0 when the
//                  source doesn't expose a total (e.g. arbitrary MCAP file).
using ProgressCallback =
    std::function<void(int64_t rows, int64_t bytes, int64_t total_bytes)>;

struct ServerVersion {
    std::string version;   // canonical string, e.g. "0.3.0"
    uint64_t major = 0;
    uint64_t minor = 0;
    uint64_t patch = 0;
    std::string pre;       // empty when no pre-release tag
};

// Server-side notification record (see sequence_notification_* and
// topic_notification_* actions).
struct Notification {
    std::string name;              // sequence name or "seq/topic" locator
    std::string type;              // e.g. "error"
    std::string msg;
    std::string created_datetime;  // ISO-ish string as the server sent it
};

// Permission tiers for API keys. Bit-cumulative on the server:
// Read < Write < Delete < Manage.
enum class ApiKeyPermission {
    Read,
    Write,
    Delete,
    Manage,
};

inline const char* toString(ApiKeyPermission p) {
    switch (p) {
        case ApiKeyPermission::Read:   return "read";
        case ApiKeyPermission::Write:  return "write";
        case ApiKeyPermission::Delete: return "delete";
        case ApiKeyPermission::Manage: return "manage";
    }
    return "read";
}

// Metadata about an API key. `api_key_token` is only populated when the key
// is first created; `api_key_status` returns everything but the token.
struct ApiKeyInfo {
    std::string api_key_token;        // set on create, empty elsewhere
    std::string api_key_fingerprint;  // server identifier (8 hex chars)
    std::string description;
    int64_t created_at_ns = 0;
    std::optional<int64_t> expires_at_ns;
};

} // namespace mosaico
