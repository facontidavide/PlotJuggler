// src/flight/mosaico_client.cpp
#include "flight/mosaico_client.hpp"

#include "flight/json_utils.hpp"
#include "flight/metadata.hpp"
#include "flight/utils.hpp"

#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <arrow/util/byte_size.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fl = arrow::flight;
using json = nlohmann::json;

namespace mosaico {

namespace {
// Cheap stderr-level timing log. No Qt dependency, so this works whether
// the SDK is linked into the Qt plugin or a headless tool.
inline std::chrono::steady_clock::time_point sdkNow() {
    return std::chrono::steady_clock::now();
}
inline int64_t sdkElapsedMs(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               sdkNow() - start).count();
}

// Short, stable tag for the calling thread — hashed down to 4 digits so log
// lines align and `grep tid=NNNN` reliably picks out one worker's trace.
// Hash collisions among the 4 pool workers are practically nil, and a
// collision wouldn't be a correctness issue, only a readability one.
inline std::string tidStr() {
    const auto h = std::hash<std::thread::id>{}(std::this_thread::get_id());
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04u",
                  static_cast<unsigned>(h % 10000u));
    return std::string(buf);
}

// Run `task(i)` for i in [0, count) using exactly min(count, parallelism)
// worker threads. Work is dispatched via an atomic counter so the threads
// naturally load-balance regardless of per-item cost. Blocks until every
// index has been processed.
//
// Concurrency is capped by the caller so loops that each call into
// pool_.checkout() won't force the pool to create overflow connections.
// `task` must be thread-safe with respect to itself; it also must not throw
// (errors should be captured in the task's own output slot).
template <typename F>
void parallelIndexed(size_t count, size_t parallelism, F&& task) {
    if (count == 0) return;
    const size_t n = std::min<size_t>(count, std::max<size_t>(1, parallelism));
    if (n == 1) {
        for (size_t i = 0; i < count; ++i) task(i);
        return;
    }
    std::atomic<size_t> next(0);
    std::vector<std::thread> workers;
    workers.reserve(n);
    for (size_t w = 0; w < n; ++w) {
        workers.emplace_back([&]() {
            while (true) {
                const size_t i = next.fetch_add(1, std::memory_order_relaxed);
                if (i >= count) return;
                task(i);
            }
        });
    }
    for (auto& t : workers) t.join();
}
}  // namespace

namespace {

// Decode a protobuf-encoded ticket to extract the resource locator string.
// The Mosaico server packs the resource locator inside the Flight ticket bytes
// as a length-prefixed string, optionally preceded by a protobuf field tag.
// Observed formats:
//   A) <varint length> <resource UTF-8> [trailing bytes]
//   B) 0x0a <varint length> <resource UTF-8> [trailing bytes]
std::string extractTicketResource(const std::string& ticket_bytes) {
    if (ticket_bytes.size() < 2) return ticket_bytes;

    for (std::size_t start : {std::size_t(1), std::size_t(0)}) {
        if (start >= ticket_bytes.size()) continue;

        std::size_t pos = start;
        uint64_t length = 0;
        unsigned shift = 0;
        bool overflow = false;
        while (pos < ticket_bytes.size()) {
            // A varint encoding a uint64 is at most 10 bytes (ceil(64/7)).
            // Anything further would make the shift >= 64 and invoke UB
            // per [expr.shift]. Bail the candidate instead of shifting.
            if (shift >= 64) {
                overflow = true;
                break;
            }
            auto b = static_cast<uint8_t>(ticket_bytes[pos]);
            ++pos;
            length |= static_cast<uint64_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
        }
        if (overflow) continue;

        if (length == 0 || length > 4096 ||
            pos + length > ticket_bytes.size())
            continue;

        std::string candidate =
            ticket_bytes.substr(pos, static_cast<std::size_t>(length));

        bool valid = true;
        for (unsigned char c : candidate) {
            if (c < 0x20 || c > 0x7E) {
                valid = false;
                break;
            }
        }
        if (valid) return candidate;
    }

    return ticket_bytes;
}

// Parse "seq/topic" -> topic_name (with leading slash restored).
// The fallback branch intentionally requires the byte at sequence_name.size()
// to be a literal '/' — without that check, `sequence_name="run"` matched
// against `resource="run42/imu"` would fabricate a topic name "/2/imu" by
// silently swallowing the '4'. Hitting such a collision is possible when the
// server hosts sequences whose names are strict prefixes of each other.
std::string extractTopicFromResource(const std::string& resource,
                                     const std::string& sequence_name) {
    std::string prefix = sequence_name + "/";
    if (resource.rfind(prefix, 0) == 0) {
        return "/" + resource.substr(prefix.size());
    }
    if (resource.size() > sequence_name.size() &&
        resource[sequence_name.size()] == '/' &&
        resource.rfind(sequence_name, 0) == 0) {
        return "/" + resource.substr(sequence_name.size() + 1);
    }
    return resource;
}

// Resource/topic names returned by the server are echoed back into later
// RPC descriptors via json.dump(). Any byte outside printable ASCII will
// either throw from json.dump (invalid UTF-8) or — if it reaches gRPC —
// trip validate_metadata. Reject such names at ingestion so a hostile or
// corrupted server cannot crash the client.
bool isSafeResourceName(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

// Parse endpoint app_metadata JSON for per-topic info.
// The server packs topic info under `info`:
//   { "resource_locator": "...",
//     "info": { "total_bytes": N, "chunks_number": N,
//               "timestamp": { "start_ns": ns, "end_ns": ns }, ... },
//     "created_at_ns": ns, "locked": bool }
//
// Each field is read in a separate step so a breakpoint can land on any
// intermediate value. A missing field leaves the corresponding out-param
// untouched at its caller-supplied default.
bool parseEndpointInfo(const std::string& metadata,
                       int64_t& ts_min, int64_t& ts_max,
                       int64_t& total_bytes) {
    if (metadata.empty()) return false;

    try {
        // Step 1: parse the app_metadata blob.
        const json root = json::parse(metadata);

        // Step 2: locate the top-level "info" object.
        const json* info = tryGetObject(root, "info");
        if (info == nullptr) return false;

        // Step 3: read total_bytes if present.
        const auto bytes_opt = tryGetInt64(*info, "total_bytes");
        if (bytes_opt.has_value()) {
            total_bytes = *bytes_opt;
        }

        // Step 4: read the nested "timestamp" object, if present.
        const json* timestamp = tryGetObject(*info, "timestamp");
        if (timestamp == nullptr) return true;

        // Step 5: extract min/max from the timestamp object.
        const auto start_opt = tryGetInt64(*timestamp, "start_ns");
        if (start_opt.has_value()) {
            ts_min = *start_opt;
        }

        const auto end_opt = tryGetInt64(*timestamp, "end_ns");
        if (end_opt.has_value()) {
            ts_max = *end_opt;
        }

        return true;
    } catch (const json::exception&) {
        return false;
    }
}

// Parse the full TopicAppMetadata blob into a TopicInfo. Fills every field
// the server advertises (see mosaicod-marshal/src/flight.rs TopicAppMetadata):
// created_at_ns, completed_at_ns, locked, resource_locator, plus the nested
// info.{total_bytes, chunks_number, timestamp}.
// Does not touch TopicInfo fields populated from other sources (topic_name,
// ticket_bytes, schema, ontology_tag, user_metadata).
bool parseTopicAppMetadata(const std::string& metadata, TopicInfo& out) {
    if (metadata.empty()) return false;

    try {
        const json root = json::parse(metadata);

        if (auto v = tryGetInt64(root, "created_at_ns"); v.has_value()) {
            out.created_at_ns = *v;
        }
        if (auto v = tryGetInt64(root, "completed_at_ns"); v.has_value()) {
            out.completed_at_ns = *v;
        }
        if (auto v = tryGetBool(root, "locked"); v.has_value()) {
            out.locked = *v;
        }
        if (auto v = tryGetString(root, "resource_locator"); v.has_value()) {
            out.resource_locator = *v;
        }

        const json* info = tryGetObject(root, "info");
        if (info != nullptr) {
            if (auto v = tryGetInt64(*info, "total_bytes"); v.has_value()) {
                out.total_size_bytes = *v;
            }
            if (auto v = tryGetInt64(*info, "chunks_number"); v.has_value()) {
                out.chunks_number = *v;
            }
            if (const json* ts = tryGetObject(*info, "timestamp"); ts != nullptr) {
                if (auto v = tryGetInt64(*ts, "start_ns"); v.has_value()) {
                    out.min_ts_ns = *v;
                }
                if (auto v = tryGetInt64(*ts, "end_ns"); v.has_value()) {
                    out.max_ts_ns = *v;
                }
            }
        }
        return true;
    } catch (const json::exception&) {
        return false;
    }
}

// Parse the top-level FlightInfo.app_metadata (SequenceAppMetadata) into a
// SequenceInfo. Fills created_at_ns, resource_locator, and the sessions
// vector. See mosaicod-marshal/src/flight.rs SequenceAppMetadata.
bool parseSequenceAppMetadata(const std::string& metadata, SequenceInfo& out) {
    if (metadata.empty()) return false;

    try {
        const json root = json::parse(metadata);

        if (auto v = tryGetInt64(root, "created_at_ns"); v.has_value()) {
            out.created_at_ns = *v;
        }
        if (auto v = tryGetString(root, "resource_locator"); v.has_value()) {
            out.resource_locator = *v;
        }

        const json* sessions = tryGetArray(root, "sessions");
        if (sessions != nullptr) {
            out.sessions.reserve(sessions->size());
            for (const auto& s : *sessions) {
                SessionInfo si;
                if (auto v = tryGetString(s, "uuid"); v.has_value()) {
                    si.uuid = *v;
                }
                if (auto v = tryGetInt64(s, "created_at_ns"); v.has_value()) {
                    si.created_at_ns = *v;
                }
                if (auto v = tryGetInt64(s, "completed_at_ns"); v.has_value()) {
                    si.completed_at_ns = *v;
                }
                if (auto v = tryGetBool(s, "locked"); v.has_value()) {
                    si.locked = *v;
                }
                if (const json* topics = tryGetArray(s, "topics"); topics != nullptr) {
                    si.topics.reserve(topics->size());
                    for (const auto& t : *topics) {
                        if (t.is_string()) si.topics.push_back(t.get<std::string>());
                    }
                }
                out.sessions.push_back(std::move(si));
            }
        }
        return true;
    } catch (const json::exception&) {
        return false;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

MosaicoClient::MosaicoClient(const std::string& server_uri,
                              int timeout_seconds, size_t pool_size,
                              const std::string& tls_cert_path,
                              const std::string& api_key)
    : pool_(server_uri, timeout_seconds, pool_size, tls_cert_path, api_key),
      timeout_(timeout_seconds) {}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

fl::FlightCallOptions MosaicoClient::callOpts() const {
    fl::FlightCallOptions opts;
    opts.timeout = fl::TimeoutDuration{static_cast<double>(timeout_)};
    return opts;
}

arrow::Status MosaicoClient::doAction(fl::FlightClient* client,
                                      const std::string& action_type,
                                      const std::string& json_body,
                                      std::string* response) {
    fl::Action action;
    action.type = action_type;
    action.body = arrow::Buffer::FromString(json_body);

    auto opts = callOpts();
    ARROW_ASSIGN_OR_RAISE(auto results, client->DoAction(opts, action));

    std::string response_data;
    while (true) {
        ARROW_ASSIGN_OR_RAISE(auto result, results->Next());
        if (!result) break;
        response_data.append(
            reinterpret_cast<const char*>(result->body->data()),
            result->body->size());
    }

    if (response_data.empty() && response) {
        return arrow::Status::IOError("DoAction '", action_type,
                                      "' returned empty response");
    }

    if (response) {
        *response = std::move(response_data);
    }

    return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// Server metadata
// ---------------------------------------------------------------------------

arrow::Result<ServerVersion> MosaicoClient::version() {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());

    std::string response;
    ARROW_RETURN_NOT_OK(doAction(&*conn, "version", "{}", &response));

    if (response.empty()) {
        return arrow::Status::IOError("version returned empty response");
    }

    try {
        // Step 1: parse the raw response and unwrap the {"response": ...} shell.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);
        if (!payload.is_object()) {
            return arrow::Status::IOError("version response not an object: ",
                                          response);
        }

        // Step 2: extract each scalar independently.
        ServerVersion v;

        const auto version_str = tryGetString(payload, "version");
        if (version_str.has_value()) {
            v.version = *version_str;
        }

        // Step 3: drop into the "semver" sub-object if present.
        const json* semver = tryGetObject(payload, "semver");
        if (semver == nullptr) return v;

        const auto major = tryGetUint64(*semver, "major");
        if (major.has_value()) v.major = *major;

        const auto minor = tryGetUint64(*semver, "minor");
        if (minor.has_value()) v.minor = *minor;

        const auto patch = tryGetUint64(*semver, "patch");
        if (patch.has_value()) v.patch = *patch;

        const auto pre = tryGetString(*semver, "pre");
        if (pre.has_value()) v.pre = *pre;

        return v;
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed version response: ", e.what());
    }
}

arrow::Result<QueryResponse> MosaicoClient::query(
    const std::vector<QueryFilter>& filters) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());

    std::string request_dump;
    try {
        // Server expects {"<filter_name>": <body_json>, ...}
        json request_payload = json::object();
        for (const auto& f : filters) {
            request_payload[f.name] = json::parse(f.body_json);
        }
        request_dump = request_payload.dump();
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed query filter: ", e.what());
    }

    std::string response;
    ARROW_RETURN_NOT_OK(
        doAction(&*conn, "query", request_dump, &response));

    QueryResponse out;
    if (response.empty()) return out;

    try {
        // Step 1: parse and unwrap.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);
        if (payload.is_null()) return out;

        // Step 2: locate the "items" array.
        const json* items = tryGetArray(payload, "items");
        if (items == nullptr) {
            return arrow::Status::IOError(
                "query response missing items array: ", response);
        }

        // Step 3: iterate items — each yields one QueryResponseItem.
        for (const auto& item : *items) {
            if (!item.is_object()) continue;

            QueryResponseItem qi;

            // Step 3a: sequence name.
            const auto seq_name = tryGetString(item, "sequence");
            if (seq_name.has_value()) {
                qi.sequence = *seq_name;
            }

            // Step 3b: per-topic list.
            const json* topics = tryGetArray(item, "topics");
            if (topics != nullptr) {
                for (const auto& t : *topics) {
                    if (!t.is_object()) continue;

                    QueryResponseTopic qt;

                    const auto locator = tryGetString(t, "locator");
                    if (locator.has_value()) {
                        qt.locator = *locator;
                    }

                    // timestamp_range: a 2-element [start_ns, end_ns] array.
                    const json* range = tryGetArray(t, "timestamp_range");
                    if (range != nullptr && range->size() == 2) {
                        const auto& start_node = (*range)[0];
                        const auto& end_node = (*range)[1];
                        if (start_node.is_number_integer()) {
                            qt.ts_start_ns = start_node.get<int64_t>();
                        }
                        if (end_node.is_number_integer()) {
                            qt.ts_end_ns = end_node.get<int64_t>();
                        }
                    }

                    qi.topics.push_back(std::move(qt));
                }
            }

            out.items.push_back(std::move(qi));
        }

        return out;
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed query response: ", e.what());
    }
}

// ---------------------------------------------------------------------------
// Discovery
// ---------------------------------------------------------------------------

arrow::Result<std::vector<SequenceInfo>> MosaicoClient::listSequences() {
    const auto t_begin = sdkNow();
    auto opts = callOpts();

    // ListFlights only yields sequence names — endpoint metadata is empty
    // on this RPC. Size and timestamps require a per-sequence GetFlightInfo,
    // which runs in parallel below. We scope `conn` tightly so it returns
    // to the pool before the parallel loop starts checking out connections.
    std::vector<std::string> names;
    int64_t ms_list = 0;
    {
        ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
        const auto t_list = sdkNow();
        ARROW_ASSIGN_OR_RAISE(auto listing, conn->ListFlights(opts, {}));
        while (true) {
            ARROW_ASSIGN_OR_RAISE(auto info, listing->Next());
            if (!info) break;
            for (const auto& path_component : info->descriptor().path) {
                // Silently drop any sequence whose name contains bytes that
                // would crash later RPCs. A noisy server can't kill us.
                if (!isSafeResourceName(path_component)) continue;
                names.push_back(path_component);
            }
        }
        ms_list = sdkElapsedMs(t_list);
    }
    std::fprintf(stderr,
                 "[Mosaico SDK] listSequences: ListFlights=%lld ms, %lld names\n",
                 (long long)ms_list, (long long)names.size());

    // Pre-size the output so worker threads can write without locks. Each
    // slot is touched by exactly one worker — no sharing, no data race.
    std::vector<SequenceInfo> sequences(names.size());
    for (size_t i = 0; i < names.size(); ++i) sequences[i].name = names[i];

    std::atomic<int64_t> getinfo_ok(0);
    std::atomic<int64_t> getinfo_fail(0);

    const auto t_getinfo_all = sdkNow();
    parallelIndexed(names.size(), pool_.poolSize(), [&](size_t i) {
        const auto t_pick = sdkNow();
        auto conn_result = pool_.checkout();
        if (!conn_result.ok()) {
            getinfo_fail.fetch_add(1, std::memory_order_relaxed);
            std::fprintf(stderr,
                         "[Mosaico SDK] listSequences: tid=%s seq[%zu]=%s "
                         "checkout FAILED\n",
                         tidStr().c_str(), i, names[i].c_str());
            return;
        }
        auto& local_conn = *conn_result;

        auto descriptor = fl::FlightDescriptor::Command(
            json{{"resource_locator", names[i]}}.dump());
        const auto t_info = sdkNow();
        auto info_result = local_conn->GetFlightInfo(opts, descriptor);
        const auto ms_info = sdkElapsedMs(t_info);
        if (!info_result.ok()) {
            getinfo_fail.fetch_add(1, std::memory_order_relaxed);
            std::fprintf(stderr,
                         "[Mosaico SDK] listSequences: tid=%s seq[%zu]=%s "
                         "GetFlightInfo FAILED (%lld ms): %s\n",
                         tidStr().c_str(), i, names[i].c_str(),
                         (long long)ms_info,
                         info_result.status().ToString().c_str());
            return;
        }
        getinfo_ok.fetch_add(1, std::memory_order_relaxed);
        auto& info = *info_result;
        std::fprintf(stderr,
                     "[Mosaico SDK] listSequences: tid=%s seq[%zu]=%s "
                     "GetFlightInfo=%lld ms (held=%lld ms)\n",
                     tidStr().c_str(), i, names[i].c_str(),
                     (long long)ms_info,
                     (long long)sdkElapsedMs(t_pick));

        SequenceInfo& seq = sequences[i];
        for (const auto& ep : info->endpoints()) {
            int64_t ts_min = 0, ts_max = 0, bytes = 0;
            if (!parseEndpointInfo(ep.app_metadata, ts_min, ts_max, bytes)) {
                continue;
            }
            seq.total_size_bytes += bytes;
            if (ts_min > 0 && (seq.min_ts_ns == 0 || ts_min < seq.min_ts_ns)) {
                seq.min_ts_ns = ts_min;
            }
            if (ts_max > seq.max_ts_ns) {
                seq.max_ts_ns = ts_max;
            }
        }

        auto schema_result = info->GetSchema(nullptr);
        if (schema_result.ok() && *schema_result) {
            auto meta = std::const_pointer_cast<arrow::KeyValueMetadata>(
                (*schema_result)->metadata());
            seq.user_metadata = extractUserMetadata(meta);
        }

        // Top-level FlightInfo.app_metadata is SequenceAppMetadata (created_at,
        // sessions list). Captured for future use; no consumer today.
        parseSequenceAppMetadata(info->app_metadata(), seq);
    });
    const auto ms_getinfo_all = sdkElapsedMs(t_getinfo_all);
    const auto ms_total = sdkElapsedMs(t_begin);
    std::fprintf(stderr,
                 "[Mosaico SDK] listSequences: per-seq GetFlightInfo=%lld ms "
                 "(%lld ok, %lld fail, avg %.1f ms, parallelism=%zu), total=%lld ms\n",
                 (long long)ms_getinfo_all,
                 (long long)getinfo_ok.load(),
                 (long long)getinfo_fail.load(),
                 names.empty() ? 0.0
                               : static_cast<double>(ms_getinfo_all) / names.size(),
                 pool_.poolSize(),
                 (long long)ms_total);

    return sequences;
}

arrow::Result<std::vector<TopicInfo>> MosaicoClient::listTopics(
    const std::string& sequence_name) {
    const auto t_begin = sdkNow();
    auto opts = callOpts();

    // Single GetFlightInfo on the sequence: each endpoint in the response
    // already carries the topic's name + timestamp range + total_bytes in its
    // app_metadata, so the topic list is fully populated from one RPC. The
    // per-topic schema / ontology / user_metadata used by the info pane are
    // fetched on demand via getTopicMetadata() — keeping them off the open-
    // sequence critical path.

    std::vector<TopicInfo> topics;
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    auto descriptor = fl::FlightDescriptor::Command(
        json{{"resource_locator", sequence_name}}.dump());

    const auto t_info = sdkNow();
    ARROW_ASSIGN_OR_RAISE(auto info, conn->GetFlightInfo(opts, descriptor));
    const auto ms_info = sdkElapsedMs(t_info);
    const size_t endpoint_count = info->endpoints().size();

    topics.reserve(endpoint_count);
    for (const auto& ep : info->endpoints()) {
        TopicInfo topic;
        topic.ticket_bytes = ep.ticket.ticket;
        std::string resource = extractTicketResource(ep.ticket.ticket);
        topic.topic_name = extractTopicFromResource(resource, sequence_name);
        if (!isSafeResourceName(topic.topic_name)) continue;

        parseTopicAppMetadata(ep.app_metadata, topic);
        topics.push_back(std::move(topic));
    }

    std::fprintf(stderr,
                 "[Mosaico SDK] listTopics %s: GetFlightInfo=%lld ms, "
                 "OK in %lld ms, %lld endpoints, %lld topics\n",
                 sequence_name.c_str(), (long long)ms_info,
                 (long long)sdkElapsedMs(t_begin),
                 (long long)endpoint_count, (long long)topics.size());
    return topics;
}

arrow::Result<TopicInfo> MosaicoClient::getTopicMetadata(
    const std::string& sequence_name,
    const std::string& topic_name) {
    const auto t_begin = sdkNow();
    auto opts = callOpts();

    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    auto descriptor = fl::FlightDescriptor::Command(
        json{{"resource_locator", packResource(sequence_name, topic_name)}}
            .dump());

    const auto t_info = sdkNow();
    ARROW_ASSIGN_OR_RAISE(auto info, conn->GetFlightInfo(opts, descriptor));
    const auto ms_info = sdkElapsedMs(t_info);

    TopicInfo topic;
    topic.topic_name = topic_name;
    if (!info->endpoints().empty()) {
        topic.ticket_bytes = info->endpoints()[0].ticket.ticket;
        int64_t ts_min = 0, ts_max = 0, bytes = 0;
        if (parseEndpointInfo(info->endpoints()[0].app_metadata,
                              ts_min, ts_max, bytes)) {
            topic.min_ts_ns = ts_min;
            topic.max_ts_ns = ts_max;
        }
    }

    auto schema_result = info->GetSchema(nullptr);
    if (schema_result.ok() && *schema_result) {
        topic.schema = *schema_result;
        auto metadata = std::const_pointer_cast<arrow::KeyValueMetadata>(
            topic.schema->metadata());
        auto tag = extractOntologyTag(metadata);
        if (tag.has_value()) topic.ontology_tag = *tag;
        topic.user_metadata = extractUserMetadata(metadata);
    }

    std::fprintf(stderr,
                 "[Mosaico SDK] getTopicMetadata %s/%s: GetFlightInfo=%lld ms, "
                 "OK in %lld ms\n",
                 sequence_name.c_str(), topic_name.c_str(),
                 (long long)ms_info, (long long)sdkElapsedMs(t_begin));
    return topic;
}

// ---------------------------------------------------------------------------
// Pull
// ---------------------------------------------------------------------------

arrow::Result<PullResult> MosaicoClient::pullTopic(
    const std::string& sequence_name,
    const std::string& topic_name,
    const TimeRange& range,
    ProgressCallback progress_cb,
    std::atomic<bool>* interrupted) {
    auto is_cancelled = [interrupted]() {
        return interrupted && interrupted->load(std::memory_order_relaxed);
    };
    const auto t_begin = sdkNow();
    const std::string tid = tidStr();
    std::fprintf(stderr,
                 "[Mosaico SDK] pullTopic %s/%s: tid=%s begin\n",
                 sequence_name.c_str(), topic_name.c_str(), tid.c_str());

    const auto t_checkout = sdkNow();
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    const auto ms_checkout = sdkElapsedMs(t_checkout);
    auto opts = callOpts();

    // Build resource locator for the topic.
    std::string resource = packResource(sequence_name, topic_name);

    // Build GetFlightInfo command, with optional time range filter.
    json cmd = {{"resource_locator", resource}};
    if (range.start_ns.has_value()) {
        cmd["timestamp_ns_start"] = *range.start_ns;
    }
    if (range.end_ns.has_value()) {
        cmd["timestamp_ns_end"] = *range.end_ns;
    }

    auto descriptor = fl::FlightDescriptor::Command(cmd.dump());
    const auto t_info = sdkNow();
    ARROW_ASSIGN_OR_RAISE(auto info, conn->GetFlightInfo(opts, descriptor));
    const auto ms_info = sdkElapsedMs(t_info);

    // Need at least one endpoint to get the ticket.
    if (info->endpoints().empty()) {
        return arrow::Status::IOError(
            "GetFlightInfo returned no endpoints for topic: ", resource);
    }

    fl::Ticket ticket;
    ticket.ticket = info->endpoints()[0].ticket.ticket;

    // The server's app_metadata `info.total_bytes` is the on-disk Parquet
    // (compressed, columnar) byte size for the WHOLE topic, irrespective of
    // any time-range filter. The bytes we accumulate below are uncompressed
    // Arrow IPC buffer sizes for the requested SLICE. The two are not in the
    // same units and cannot be compared, so we don't pass a denominator here:
    // `progress_cb` receives 0 as the total, which the plugin renders as a
    // bytes-only progress line (no fraction, no percentage). Restore a real
    // denominator only once the server advertises a slice-aware,
    // wire-format-aware byte total.
    std::fprintf(stderr,
                 "[Mosaico SDK] pullTopic %s/%s: tid=%s checkout=%lld ms, "
                 "GetFlightInfo=%lld ms\n",
                 sequence_name.c_str(), topic_name.c_str(), tid.c_str(),
                 (long long)ms_checkout, (long long)ms_info);

    // DoGet is a stream of unknown size — a gRPC deadline is an absolute
    // wall-clock bound on the whole call, so applying `timeout_` here aborts
    // big pulls midway with DEADLINE_EXCEEDED even while data is flowing.
    // The Python SDK doesn't pass a deadline to do_get either; match that:
    // streaming is bounded by the connection's keepalive/liveness, not a
    // pre-declared total duration.
    fl::FlightCallOptions stream_opts;
    const auto t_doget = sdkNow();
    ARROW_ASSIGN_OR_RAISE(auto reader, conn->DoGet(stream_opts, ticket));
    const auto ms_doget = sdkElapsedMs(t_doget);

    PullResult result;
    const auto t_schema = sdkNow();
    ARROW_ASSIGN_OR_RAISE(result.schema, reader->GetSchema());
    const auto ms_schema = sdkElapsedMs(t_schema);

    int64_t total_rows = 0;
    int64_t total_bytes = 0;
    int64_t chunk_count = 0;
    int64_t ms_first_chunk = -1;       // time-to-first-byte from DoGet start
    int64_t max_gap_ms = 0;             // largest gap between two consecutive Next() completions
    auto last_chunk_time = sdkNow();
    const auto t_stream_begin = last_chunk_time;

    while (true) {
        if (is_cancelled()) {
            reader->Cancel();
            std::fprintf(stderr,
                         "[Mosaico SDK] pullTopic %s/%s: tid=%s CANCELLED after %lld ms, "
                         "%lld chunks, %lld bytes\n",
                         sequence_name.c_str(), topic_name.c_str(), tid.c_str(),
                         (long long)sdkElapsedMs(t_begin), (long long)chunk_count,
                         (long long)total_bytes);
            return arrow::Status::Cancelled("pull cancelled by caller");
        }
        const auto t_next = sdkNow();
        ARROW_ASSIGN_OR_RAISE(auto chunk, reader->Next());
        const auto t_got = sdkNow();
        if (!chunk.data) break;
        if (chunk.data->num_rows() == 0) continue;

        ++chunk_count;
        if (ms_first_chunk < 0) {
            ms_first_chunk = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 t_got - t_stream_begin).count();
        } else {
            const auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 t_got - last_chunk_time).count();
            if (gap > max_gap_ms) max_gap_ms = gap;
        }
        last_chunk_time = t_got;

        total_rows += chunk.data->num_rows();
        // Walk every buffer reachable from the batch — including child
        // buffers under struct/list/map columns and per-column dictionaries.
        // The previous flat loop over `column->data()->buffers` only saw
        // top-level buffers, so nested types (e.g. ROS Odometry's
        // pose/twist/covariance structs) were under-counted by ~10x and the
        // download progress numerator was effectively meaningless on those
        // topics. arrow::util::TotalBufferSize matches what the Python
        // RecordBatch.get_total_buffer_size() reports.
        total_bytes += arrow::util::TotalBufferSize(*chunk.data);

        result.batches.push_back(chunk.data);

        if (progress_cb) progress_cb(total_rows, total_bytes, /*total=*/0);
    }

    const auto ms_stream = sdkElapsedMs(t_stream_begin);
    const auto ms_total = sdkElapsedMs(t_begin);
    const double mb = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    const double mb_per_sec = ms_stream > 0 ? (mb * 1000.0 / ms_stream) : 0.0;
    std::fprintf(stderr,
                 "[Mosaico SDK] pullTopic %s/%s: tid=%s DoGet=%lld ms, GetSchema=%lld ms, "
                 "stream=%lld ms (%lld chunks, %lld rows, %.2f MB, %.2f MB/s), "
                 "first-chunk=%lld ms, max-gap=%lld ms, total=%lld ms\n",
                 sequence_name.c_str(), topic_name.c_str(), tid.c_str(),
                 (long long)ms_doget, (long long)ms_schema, (long long)ms_stream,
                 (long long)chunk_count, (long long)total_rows, mb, mb_per_sec,
                 (long long)ms_first_chunk, (long long)max_gap_ms,
                 (long long)ms_total);

    return result;
}

arrow::Status MosaicoClient::pullTopics(
    const std::string& sequence_name,
    const std::vector<std::string>& topic_names,
    const TimeRange& range,
    TopicCompleteCallback on_topic_done,
    MultiTopicProgressCallback progress_cb,
    std::atomic<bool>* interrupted) {
    const auto t_begin = sdkNow();
    const size_t parallelism = std::min<size_t>(topic_names.size(), pool_.poolSize());
    {
        std::string topics_csv;
        for (size_t i = 0; i < topic_names.size(); ++i) {
            if (i) topics_csv += ", ";
            topics_csv += topic_names[i];
        }
        std::fprintf(stderr,
                     "[Mosaico SDK] pullTopics %s: dispatch %lld topics, "
                     "parallelism=%zu (pool_size=%zu), dispatch_tid=%s, "
                     "topics=[%s]\n",
                     sequence_name.c_str(), (long long)topic_names.size(),
                     parallelism, pool_.poolSize(), tidStr().c_str(),
                     topics_csv.c_str());
    }

    // on_topic_done is invoked from worker threads; serialize so the
    // consumer (typically the Qt plugin, which just emits a queued signal)
    // sees one call at a time. progress_cb is *not* serialized — it fires
    // mid-stream and the worker would otherwise block under the mutex,
    // wasting pool capacity. Consumers should make progress_cb cheap and
    // thread-safe (e.g. emit a queued Qt signal, no shared state).
    std::mutex done_mutex;
    // Counter visible across worker threads so we can log how many topics
    // each tid actually picked up — confirms the parallelIndexed counter
    // is sharing work, not pinning each topic to a fresh thread.
    std::atomic<size_t> picked_count(0);

    parallelIndexed(topic_names.size(), pool_.poolSize(), [&](size_t i) {
        const std::string& topic = topic_names[i];
        const auto t_pick = sdkNow();
        const size_t my_pick = picked_count.fetch_add(1, std::memory_order_relaxed) + 1;
        std::fprintf(stderr,
                     "[Mosaico SDK] pullTopics %s: tid=%s picked topic[%zu]=%s "
                     "(pick #%zu of %lld) at +%lld ms\n",
                     sequence_name.c_str(), tidStr().c_str(), i,
                     topic.c_str(), my_pick,
                     (long long)topic_names.size(),
                     (long long)sdkElapsedMs(t_begin));

        // Wrap the multi-topic progress callback so pullTopic's per-call
        // signature doesn't need to know about topic names.
        ProgressCallback per_topic_progress;
        if (progress_cb) {
            per_topic_progress = [&progress_cb, &topic](
                                     int64_t rows, int64_t bytes, int64_t total) {
                progress_cb(topic, rows, bytes, total);
            };
        }

        auto result = pullTopic(sequence_name, topic, range,
                                std::move(per_topic_progress), interrupted);

        std::fprintf(stderr,
                     "[Mosaico SDK] pullTopics %s: tid=%s released topic[%zu]=%s "
                     "after %lld ms (status=%s)\n",
                     sequence_name.c_str(), tidStr().c_str(), i,
                     topic.c_str(), (long long)sdkElapsedMs(t_pick),
                     result.ok() ? "ok" : result.status().ToString().c_str());

        if (on_topic_done) {
            std::lock_guard<std::mutex> lg(done_mutex);
            on_topic_done(topic, std::move(result));
        }
    });

    std::fprintf(stderr,
                 "[Mosaico SDK] pullTopics %s: OK in %lld ms (dispatched %lld topics)\n",
                 sequence_name.c_str(), (long long)sdkElapsedMs(t_begin),
                 (long long)topic_names.size());

    return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// Notifications
// ---------------------------------------------------------------------------

namespace {
std::vector<Notification> parseNotifications(const std::string& response) {
    std::vector<Notification> out;
    if (response.empty()) return out;

    try {
        // Step 1: parse and unwrap the envelope.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);
        if (payload.is_null()) return out;

        // Step 2: locate the notifications array.
        const json* notifications = tryGetArray(payload, "notifications");
        if (notifications == nullptr) return out;

        // Step 3: extract each notification one field at a time.
        for (const auto& n : *notifications) {
            if (!n.is_object()) continue;

            Notification entry;

            const auto name = tryGetString(n, "name");
            if (name.has_value()) entry.name = *name;

            const auto type = tryGetString(n, "notification_type");
            if (type.has_value()) entry.type = *type;

            const auto msg = tryGetString(n, "msg");
            if (msg.has_value()) entry.msg = *msg;

            const auto created = tryGetString(n, "created_datetime");
            if (created.has_value()) entry.created_datetime = *created;

            out.push_back(std::move(entry));
        }
    } catch (const json::exception&) {
        // Malformed server response: caller sees an empty list rather than
        // a crash. This matches the "server says nothing" outcome.
        out.clear();
    }
    return out;
}
}  // namespace

arrow::Status MosaicoClient::reportSequenceNotification(
    const std::string& sequence_name, const std::string& type,
    const std::string& message) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", sequence_name},
                    {"notification_type", type},
                    {"msg", message}};
    return doAction(&*conn, "sequence_notification_create", payload.dump());
}

arrow::Status MosaicoClient::reportTopicNotification(
    const std::string& sequence_name, const std::string& topic_name,
    const std::string& type, const std::string& message) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", packResource(sequence_name, topic_name)},
                    {"notification_type", type},
                    {"msg", message}};
    return doAction(&*conn, "topic_notification_create", payload.dump());
}

arrow::Result<std::vector<Notification>>
MosaicoClient::listSequenceNotifications(const std::string& sequence_name) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", sequence_name}};
    std::string response;
    ARROW_RETURN_NOT_OK(doAction(&*conn, "sequence_notification_list",
                                 payload.dump(), &response));
    return parseNotifications(response);
}

arrow::Result<std::vector<Notification>>
MosaicoClient::listTopicNotifications(const std::string& sequence_name,
                                      const std::string& topic_name) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", packResource(sequence_name, topic_name)}};
    std::string response;
    ARROW_RETURN_NOT_OK(doAction(&*conn, "topic_notification_list",
                                 payload.dump(), &response));
    return parseNotifications(response);
}

arrow::Status MosaicoClient::purgeSequenceNotifications(
    const std::string& sequence_name) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", sequence_name}};
    return doAction(&*conn, "sequence_notification_purge", payload.dump());
}

arrow::Status MosaicoClient::purgeTopicNotifications(
    const std::string& sequence_name, const std::string& topic_name) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"locator", packResource(sequence_name, topic_name)}};
    return doAction(&*conn, "topic_notification_purge", payload.dump());
}

// ---------------------------------------------------------------------------
// API key management (Manage tier)
// ---------------------------------------------------------------------------

arrow::Result<ApiKeyInfo> MosaicoClient::createApiKey(
    ApiKeyPermission permission, const std::string& description,
    std::optional<int64_t> expires_at_ns) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());

    json request_payload = {{"permissions", toString(permission)},
                            {"description", description}};
    if (expires_at_ns.has_value()) {
        request_payload["expires_at_ns"] = *expires_at_ns;
    }

    std::string response;
    ARROW_RETURN_NOT_OK(
        doAction(&*conn, "api_key_create", request_payload.dump(), &response));

    if (response.empty()) {
        return arrow::Status::IOError("api_key_create empty response");
    }

    try {
        // Step 1: parse and unwrap.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);

        // Step 2: api_key_token is the only required field on create.
        const auto token = tryGetString(payload, "api_key_token");
        if (!token.has_value()) {
            return arrow::Status::IOError(
                "api_key_create missing api_key_token: ", response);
        }

        // Step 3: assemble the result. Fingerprint is the last 8 hex chars
        // of the token (format: msco_<32>_<8>); the server only returns
        // the full token on create, so we derive the fingerprint locally.
        ApiKeyInfo info;
        info.api_key_token = *token;
        info.description = description;

        if (info.api_key_token.size() >= 8) {
            const size_t start = info.api_key_token.size() - 8;
            info.api_key_fingerprint = info.api_key_token.substr(start);
        }

        if (expires_at_ns.has_value()) {
            info.expires_at_ns = *expires_at_ns;
        }

        return info;
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed api_key_create response: ",
                                      e.what());
    }
}

arrow::Result<ApiKeyInfo> MosaicoClient::apiKeyStatus(
    const std::string& fingerprint) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json request_payload = {{"api_key_fingerprint", fingerprint}};

    std::string response;
    ARROW_RETURN_NOT_OK(
        doAction(&*conn, "api_key_status", request_payload.dump(), &response));

    if (response.empty()) {
        return arrow::Status::IOError("api_key_status empty response");
    }

    try {
        // Step 1: parse and unwrap.
        const json root = json::parse(response);
        const json& payload = unwrapResponse(root);
        if (!payload.is_object()) {
            return arrow::Status::IOError(
                "api_key_status bad response: ", response);
        }

        // Step 2: extract each field independently.
        ApiKeyInfo info;

        const auto fingerprint = tryGetString(payload, "api_key_fingerprint");
        if (fingerprint.has_value()) {
            info.api_key_fingerprint = *fingerprint;
        }

        const auto desc = tryGetString(payload, "description");
        if (desc.has_value()) {
            info.description = *desc;
        }

        const auto created = tryGetInt64(payload, "created_at_ns");
        if (created.has_value()) {
            info.created_at_ns = *created;
        }

        const auto expires = tryGetInt64(payload, "expires_at_ns");
        if (expires.has_value()) {
            info.expires_at_ns = *expires;
        }

        return info;
    } catch (const json::exception& e) {
        return arrow::Status::Invalid("malformed api_key_status response: ",
                                      e.what());
    }
}

arrow::Status MosaicoClient::revokeApiKey(const std::string& fingerprint) {
    ARROW_ASSIGN_OR_RAISE(auto conn, pool_.checkout());
    json payload = {{"api_key_fingerprint", fingerprint}};
    return doAction(&*conn, "api_key_revoke", payload.dump());
}

} // namespace mosaico
