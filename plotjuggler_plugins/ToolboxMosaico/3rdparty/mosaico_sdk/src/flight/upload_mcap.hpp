#pragma once

#include "flight/mosaico_client.hpp"

#include <arrow/status.h>

#include <atomic>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mosaico {

// Progress callback: rows processed on the current channel, bytes processed,
// rows total for the current channel, current topic name.
using UploadProgressCallback = std::function<void(
    int64_t rows_done, int64_t bytes_done, int64_t rows_total,
    const std::string& current_topic)>;

// Per-run outcome for an MCAP upload. `topics_attempted` counts channels
// that had a resolvable ontology tag (channels without one are skipped
// silently as they are not expected to upload). `topic_failures` lists
// every topic that started but did not complete cleanly, with the first
// error message the SDK observed.
struct UploadReport {
    int topics_attempted = 0;
    int topics_succeeded = 0;
    std::vector<std::pair<std::string, std::string>> topic_failures;
};

// Upload an MCAP file end-to-end: index channels, resolve ontology tags,
// decode messages, stream batches, finalize. Per-topic failures do not
// abort the run — the function attempts every topic, finalizes at the
// end so complete topics persist, and reports failures via `report` plus
// a non-OK return status.
//
// @param client Connected MosaicoClient.
// @param mcap_path Path to the MCAP file.
// @param sequence_name Name for the new sequence (server must not have this).
// @param progress_cb Optional per-batch progress callback.
// @param interrupted Optional flag polled between batches; set to true to
//   abort (triggers cleanup instead of finalize).
// @param report Optional out-parameter capturing per-topic success/failure.
// @return OK iff every attempted topic succeeded AND finalize succeeded.
//   Cancelled if `interrupted` was set. IOError with an aggregated message
//   naming the failed topics otherwise.
arrow::Status uploadMcap(
    MosaicoClient& client,
    const std::string& mcap_path,
    const std::string& sequence_name,
    UploadProgressCallback progress_cb = nullptr,
    std::atomic<bool>* interrupted = nullptr,
    UploadReport* report = nullptr);

}  // namespace mosaico
