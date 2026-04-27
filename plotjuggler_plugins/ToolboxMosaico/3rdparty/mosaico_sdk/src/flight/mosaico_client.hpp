// src/flight/mosaico_client.hpp
#pragma once

#include "flight/connection_pool.hpp"
#include "flight/query.hpp"
#include "flight/types.hpp"

#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type_fwd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mosaico
{

class MosaicoClient
{
public:
  /// Construct a Mosaico client.
  ///
  /// @param server_uri Flight server URI. Use grpc+tls:// to enable TLS.
  /// @param timeout_seconds Per-RPC timeout.
  /// @param pool_size Number of pooled connections.
  /// @param tls_cert_path Optional path to a PEM CA certificate. When empty
  ///   and the URI uses grpc+tls://, the system root CA store is used
  ///   (one-way TLS). When set, the file's contents are used as the trust
  ///   anchor for verifying the server certificate.
  /// @param api_key Optional Mosaico API key. When non-empty, every RPC
  ///   sends a `mosaico-api-key-token` header with this value.
  MosaicoClient(const std::string& server_uri, int timeout_seconds, size_t pool_size = 4,
                const std::string& tls_cert_path = "", const std::string& api_key = "");

  // Server metadata
  arrow::Result<ServerVersion> version();

  // Metadata query — compose with QueryTopicBuilder / QuerySequenceBuilder.
  // Per Python SDK: multiple filters are ANDed (topics matching all clauses).
  arrow::Result<QueryResponse> query(const std::vector<QueryFilter>& filters);

  // Discovery
  arrow::Result<std::vector<SequenceInfo>> listSequences();

  // Lists topics in a sequence with one GetFlightInfo call. The returned
  // TopicInfo entries have `topic_name`, `min_ts_ns`, `max_ts_ns`, and
  // `ticket_bytes` filled in from the sequence-level endpoint metadata; the
  // `schema`, `ontology_tag`, and `user_metadata` fields are LEFT EMPTY here
  // and must be fetched on demand via getTopicMetadata(). This avoids the
  // 1-RPC-per-topic round-trips that previously dominated the open-sequence
  // latency.
  arrow::Result<std::vector<TopicInfo>> listTopics(const std::string& sequence_name);

  // Fetches the schema + ontology tag + user metadata for a single topic
  // via one GetFlightInfo call. Intended to be called lazily (on hover/
  // selection) so the caller pays the round-trip only for topics the user
  // actually inspects. The returned TopicInfo has all fields populated.
  arrow::Result<TopicInfo> getTopicMetadata(const std::string& sequence_name,
                                            const std::string& topic_name);

  // Pull. `interrupted` is an optional externally-owned cancellation flag —
  // checked between chunks; when set the underlying stream reader is
  // cancelled and the call returns `Status::Cancelled`. Safe to leave null.
  arrow::Result<PullResult> pullTopic(const std::string& sequence_name,
                                      const std::string& topic_name, const TimeRange& range = {},
                                      ProgressCallback progress_cb = nullptr,
                                      std::atomic<bool>* interrupted = nullptr,
                                      BatchCallback batch_cb = nullptr,
                                      SchemaCallback schema_cb = nullptr,
                                      bool retain_batches = true);

  // Callback types used by pullTopics.
  //
  // `TopicCompleteCallback` is invoked serially (under an internal mutex)
  // as each topic finishes — per-topic success or failure is delivered as
  // an arrow::Result so partial failures don't abort the batch.
  //
  // `MultiTopicProgressCallback` fires during streaming; multiple topics
  // may report concurrently, so the callback must be thread-safe (the SDK
  // does not serialize it).
  using TopicCompleteCallback =
      std::function<void(const std::string& topic_name, arrow::Result<PullResult> result)>;
  using MultiTopicProgressCallback = std::function<void(const std::string& topic_name, int64_t rows,
                                                        int64_t bytes, int64_t total_bytes)>;
  using MultiTopicBatchCallback = std::function<void(
      const std::string& topic_name, const std::shared_ptr<arrow::RecordBatch>& batch)>;
  using MultiTopicSchemaCallback = std::function<void(
      const std::string& topic_name, const std::shared_ptr<arrow::Schema>& schema)>;

  // Pull multiple topics of the same sequence in parallel (bounded by the
  // connection pool size). Blocks until every topic has completed or been
  // cancelled. Shared `interrupted` flag is observed by all workers.
  // Returns OK unless the SDK itself failed to dispatch (bad args, etc.) —
  // per-topic errors are delivered via `on_topic_done`.
  arrow::Status pullTopics(const std::string& sequence_name,
                           const std::vector<std::string>& topic_names, const TimeRange& range = {},
                           TopicCompleteCallback on_topic_done = nullptr,
                           MultiTopicProgressCallback progress_cb = nullptr,
                           std::atomic<bool>* interrupted = nullptr,
                           MultiTopicBatchCallback batch_cb = nullptr,
                           MultiTopicSchemaCallback schema_cb = nullptr,
                           bool retain_batches = true);

  // Notifications (server-side error trail).
  arrow::Status reportSequenceNotification(const std::string& sequence_name,
                                           const std::string& type, const std::string& message);
  arrow::Status reportTopicNotification(const std::string& sequence_name,
                                        const std::string& topic_name, const std::string& type,
                                        const std::string& message);
  arrow::Result<std::vector<Notification>>
  listSequenceNotifications(const std::string& sequence_name);
  arrow::Result<std::vector<Notification>> listTopicNotifications(const std::string& sequence_name,
                                                                  const std::string& topic_name);
  arrow::Status purgeSequenceNotifications(const std::string& sequence_name);
  arrow::Status purgeTopicNotifications(const std::string& sequence_name,
                                        const std::string& topic_name);

  // API key management (requires Manage tier).
  arrow::Result<ApiKeyInfo> createApiKey(ApiKeyPermission permission,
                                         const std::string& description,
                                         std::optional<int64_t> expires_at_ns = std::nullopt);
  arrow::Result<ApiKeyInfo> apiKeyStatus(const std::string& fingerprint);
  arrow::Status revokeApiKey(const std::string& fingerprint);

private:
  arrow::Status doAction(arrow::flight::FlightClient* client, const std::string& action_type,
                         const std::string& json_body, std::string* response = nullptr);
  arrow::flight::FlightCallOptions callOpts() const;

  ConnectionPool pool_;
  int timeout_;
};

}  // namespace mosaico
