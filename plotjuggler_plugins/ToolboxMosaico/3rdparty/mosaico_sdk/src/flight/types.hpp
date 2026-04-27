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

namespace mosaico
{

// Server-side recording session. Populated from SequenceAppMetadata.sessions.
// Currently captured for future use (session grouping, live/sealed UI);
// no active consumer reads these fields today.
struct SessionInfo
{
  std::string uuid;
  int64_t created_at_ns = 0;
  std::optional<int64_t> completed_at_ns;
  std::vector<std::string> topics;
  bool locked = false;
};

struct SequenceInfo
{
  std::string name;
  int64_t min_ts_ns = 0;
  int64_t max_ts_ns = 0;
  int64_t total_size_bytes = 0;
  std::unordered_map<std::string, std::string> user_metadata;
  // Fields below come from the top-level FlightInfo.app_metadata
  // (SequenceAppMetadata). Captured for future use.
  int64_t created_at_ns = 0;
  std::string resource_locator;
  std::vector<SessionInfo> sessions;
};

struct TopicInfo
{
  std::string topic_name;
  std::string ontology_tag;
  std::shared_ptr<arrow::Schema> schema;
  std::unordered_map<std::string, std::string> user_metadata;
  std::string ticket_bytes;
  int64_t min_ts_ns = 0;
  int64_t max_ts_ns = 0;
  int64_t total_size_bytes = 0;
  // Fields below come from TopicAppMetadata. Captured for future use.
  int64_t chunks_number = 0;
  int64_t created_at_ns = 0;
  std::optional<int64_t> completed_at_ns;
  bool locked = false;
  std::string resource_locator;
};

struct TimeRange
{
  std::optional<int64_t> start_ns;
  std::optional<int64_t> end_ns;
};

struct PullResult
{
  std::shared_ptr<arrow::Schema> schema;
  std::vector<std::shared_ptr<arrow::RecordBatch>> batches;
};

// Progress hook for streaming reads (both Flight pullTopic and MCAP reads).
//   rows         — cumulative rows read so far.
//   bytes        — cumulative uncompressed bytes read so far.
//   total_bytes  — comparable total in the same units as bytes, or 0 when the
//                  source does not expose one. Mosaico's physical topic size
//                  is exposed separately via TopicInfo::total_size_bytes.
using ProgressCallback = std::function<void(int64_t rows, int64_t bytes, int64_t total_bytes)>;

// Optional hooks for consumers that want to observe a streaming pull without
// waiting for the final PullResult. The SDK still owns the batches and returns
// the usual PullResult; callbacks must be cheap because they run on the pull
// worker thread.
using SchemaCallback = std::function<void(const std::shared_ptr<arrow::Schema>& schema)>;
using BatchCallback = std::function<void(const std::shared_ptr<arrow::RecordBatch>& batch)>;

struct ServerVersion
{
  std::string version;  // canonical string, e.g. "0.3.0"
  uint64_t major = 0;
  uint64_t minor = 0;
  uint64_t patch = 0;
  std::string pre;  // empty when no pre-release tag
};

// Server-side notification record (see sequence_notification_* and
// topic_notification_* actions).
struct Notification
{
  std::string name;  // sequence name or "seq/topic" locator
  std::string type;  // e.g. "error"
  std::string msg;
  std::string created_datetime;  // ISO-ish string as the server sent it
};

// Permission tiers for API keys. Bit-cumulative on the server:
// Read < Write < Delete < Manage.
enum class ApiKeyPermission
{
  Read,
  Write,
  Delete,
  Manage,
};

inline const char* toString(ApiKeyPermission p)
{
  switch (p)
  {
    case ApiKeyPermission::Read:
      return "read";
    case ApiKeyPermission::Write:
      return "write";
    case ApiKeyPermission::Delete:
      return "delete";
    case ApiKeyPermission::Manage:
      return "manage";
  }
  return "read";
}

// Metadata about an API key. `api_key_token` is only populated when the key
// is first created; `api_key_status` returns everything but the token.
struct ApiKeyInfo
{
  std::string api_key_token;        // set on create, empty elsewhere
  std::string api_key_fingerprint;  // server identifier (8 hex chars)
  std::string description;
  int64_t created_at_ns = 0;
  std::optional<int64_t> expires_at_ns;
};

}  // namespace mosaico
