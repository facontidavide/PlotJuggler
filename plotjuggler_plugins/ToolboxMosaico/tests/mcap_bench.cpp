// Standalone benchmark for the MCAP → ontology-builder path.
// Measures the local (CPU-bound) cost of the upload pipeline — no network.
// Usage:
//   mcap_bench <path.mcap> [<path.mcap> ...]
//
// For each file, reports:
//   - file size
//   - index time (summary scan)
//   - per-channel decode time, row count, resolved ontology tag
//   - aggregate rows/sec, MB/sec
//
// The decode path exactly mirrors uploadMcap() minus putBatch(), so the
// numbers reflect the throughput ceiling of the local pipeline before
// network becomes a factor.

#include "mcap_reader.hpp"
#include "ontology/ontology_registry.hpp"
#include "ontology/tag_resolver.hpp"
#include "ontology/field_map.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

using Clock = std::chrono::steady_clock;
using mosaico::ChannelInfo;
using mosaico::FieldMap;
using mosaico::McapChannelIndex;
using mosaico::OntologyBuilder;
using mosaico::OntologyRegistry;
using mosaico::createRosTagResolver;
using mosaico::decodeMcapChannel;
using mosaico::indexMcapChannels;

namespace {

struct ChannelResult {
    std::string topic;
    std::string schema;
    std::string resolved_tag;
    int64_t rows = 0;
    int64_t bytes = 0;
    double decode_ms = 0.0;
    bool attempted = false;
    bool ok = false;
};

double millis(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}

void benchFile(const std::string& path) {
    std::error_code ec;
    auto file_size = std::filesystem::file_size(path, ec);
    std::printf("\n=== %s (%.1f MB) ===\n",
                path.c_str(), static_cast<double>(file_size) / 1e6);
    if (ec) {
        std::printf("  cannot stat: %s\n", ec.message().c_str());
        return;
    }

    auto t_index_start = Clock::now();
    McapChannelIndex index;
    auto status = indexMcapChannels(path, index);
    auto t_index_end = Clock::now();
    if (!status.ok()) {
        std::printf("  index failed: %s\n", status.ToString().c_str());
        return;
    }
    std::printf("  indexed in %.1f ms — %zu channels, %lu total messages\n",
                millis(t_index_start, t_index_end),
                index.channels.size(),
                static_cast<unsigned long>(index.total_messages));

    auto resolver = createRosTagResolver();
    OntologyRegistry registry;

    int64_t agg_rows = 0;
    int64_t agg_bytes = 0;
    double agg_decode_ms = 0.0;
    size_t attempted = 0;
    size_t ok_count = 0;
    size_t skipped_no_tag = 0;

    for (const auto& channel : index.channels) {
        ChannelResult cr;
        cr.topic = channel.topic;
        cr.schema = channel.schema_name;

        auto tag = resolver.resolve(channel.schema_name);
        OntologyBuilder* builder = nullptr;
        if (tag) builder = registry.find(*tag);
        if (!builder) {
            skipped_no_tag++;
            std::printf("  - %-50s  (no tag for '%s' — skipped)\n",
                        channel.topic.c_str(), channel.schema_name.c_str());
            continue;
        }
        cr.resolved_tag = *tag;
        cr.attempted = true;
        attempted++;

        int64_t local_rows = 0;
        int64_t local_bytes = 0;
        auto t_decode_start = Clock::now();
        auto decode_status = decodeMcapChannel(
            path, channel,
            [&](const FieldMap& fields) -> arrow::Status {
                ARROW_RETURN_NOT_OK(builder->append(fields));
                // Flush the builder when it fills, matching uploadMcap().
                if (builder->shouldFlush()) {
                    auto batch = builder->flush();
                    if (!batch.ok()) return batch.status();
                }
                return arrow::Status::OK();
            },
            [&](int64_t rows, int64_t bytes, int64_t /*total_bytes*/) {
                local_rows = rows;
                local_bytes = bytes;
            });
        // Drain the tail batch so the builder internal state resets between
        // channels and we measure the full flush cost.
        auto tail = builder->flush();
        (void)tail;
        auto t_decode_end = Clock::now();

        cr.rows = local_rows;
        cr.bytes = local_bytes;
        cr.decode_ms = millis(t_decode_start, t_decode_end);
        cr.ok = decode_status.ok();

        double rows_per_sec = cr.decode_ms > 0 ? (cr.rows * 1000.0 / cr.decode_ms) : 0.0;
        double mb_per_sec = cr.decode_ms > 0 ? (cr.bytes / cr.decode_ms / 1000.0) : 0.0;
        if (cr.ok) {
            std::printf("  - %-50s  tag=%-16s  rows=%8lld  %.1f ms  %.0f rows/s  %.1f MB/s\n",
                        channel.topic.c_str(),
                        cr.resolved_tag.c_str(),
                        static_cast<long long>(cr.rows),
                        cr.decode_ms, rows_per_sec, mb_per_sec);
        } else {
            std::printf("  - %-50s  tag=%-16s  [FAILED] %s\n",
                        channel.topic.c_str(),
                        cr.resolved_tag.c_str(),
                        decode_status.ToString().c_str());
        }

        if (cr.ok) {
            ok_count++;
            agg_rows += cr.rows;
            agg_bytes += cr.bytes;
            agg_decode_ms += cr.decode_ms;
        }
    }

    std::printf("\n  Summary: %zu channels (%zu with tag, %zu no tag), %zu decoded OK\n",
                index.channels.size(), attempted, skipped_no_tag, ok_count);
    if (agg_decode_ms > 0) {
        double rows_per_sec = agg_rows * 1000.0 / agg_decode_ms;
        double mb_per_sec = agg_bytes / agg_decode_ms / 1000.0;
        std::printf("  Total: %lld rows in %.1f ms  →  %.0f rows/s  %.1f MB/s (decoded payload)\n",
                    static_cast<long long>(agg_rows), agg_decode_ms,
                    rows_per_sec, mb_per_sec);
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <path.mcap> [<path.mcap> ...]\n", argv[0]);
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        benchFile(argv[i]);
    }
    return 0;
}
