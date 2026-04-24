#pragma once

#include "flight/types.hpp"
#include "ontology/field_map.hpp"

#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/type_fwd.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mosaico {

struct ChannelInfo {
  uint16_t channel_id = 0;
  std::string topic;
  std::string encoding;
  std::string schema_name;
  std::string schema_encoding;
  uint64_t message_count = 0;
};

struct McapChannelIndex {
  std::string file_path;
  std::vector<ChannelInfo> channels;
  uint64_t total_messages = 0;
  uint64_t start_time_ns = 0;
  uint64_t end_time_ns = 0;
  uint64_t file_size_bytes = 0;
};

arrow::Status indexMcapChannels(const std::string& file_path,
                                McapChannelIndex& out);

/// Callback invoked for each decoded message. The FieldMap contains all
/// decoded fields plus "_log_time_ns" (uint64_t). Return non-OK status
/// to stop iteration early.
using DecodeCallback = std::function<arrow::Status(const FieldMap& fields)>;

/// Decode messages from a single MCAP channel, invoking on_message for each.
/// Each FieldMap includes a "_log_time_ns" key with the message log timestamp.
/// If the callback returns a non-OK status, iteration stops and that status
/// is returned.
arrow::Status decodeMcapChannel(
    const std::string& file_path,
    const ChannelInfo& channel,
    DecodeCallback on_message,
    ProgressCallback progress_cb = nullptr);

arrow::Status readMcapChannel(
    const std::string& file_path, const ChannelInfo& channel,
    std::shared_ptr<arrow::Schema>& out_schema,
    std::vector<std::shared_ptr<arrow::RecordBatch>>& out_batches,
    ProgressCallback progress_cb = nullptr);

}  // namespace mosaico
