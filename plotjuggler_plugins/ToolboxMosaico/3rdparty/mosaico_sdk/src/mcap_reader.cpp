// tools/mcap_reader.cpp
#include "mcap_reader.hpp"

#include "decoders/decoder_factory.hpp"
#include "ontology/field_map.hpp"

#include <mcap/reader.hpp>

#include <arrow/api.h>
#include <arrow/builder.h>

#include <filesystem>
#include <map>

namespace mosaico {
namespace {

constexpr int64_t kBatchSize = 65536;

// ---------------------------------------------------------------------------
// Arrow shim helpers: convert FieldMap to Arrow columns.
// Used by readMcapChannel() to build RecordBatches from FieldMaps.
// ---------------------------------------------------------------------------

// Infer Arrow type from a FieldValue.
std::shared_ptr<arrow::DataType> arrowTypeFromFieldValue(const FieldValue& fv) {
  return std::visit([](auto&& v) -> std::shared_ptr<arrow::DataType> {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, bool>) return arrow::boolean();
    else if constexpr (std::is_same_v<T, int8_t>) return arrow::int8();
    else if constexpr (std::is_same_v<T, int16_t>) return arrow::int16();
    else if constexpr (std::is_same_v<T, int32_t>) return arrow::int32();
    else if constexpr (std::is_same_v<T, int64_t>) return arrow::int64();
    else if constexpr (std::is_same_v<T, uint8_t>) return arrow::uint8();
    else if constexpr (std::is_same_v<T, uint16_t>) return arrow::uint16();
    else if constexpr (std::is_same_v<T, uint32_t>) return arrow::uint32();
    else if constexpr (std::is_same_v<T, uint64_t>) return arrow::uint64();
    else if constexpr (std::is_same_v<T, float>) return arrow::float32();
    else if constexpr (std::is_same_v<T, double>) return arrow::float64();
    else if constexpr (std::is_same_v<T, std::string>) return arrow::utf8();
    else return arrow::utf8();  // fallback for vectors etc.
  }, fv);
}

// Append a FieldValue to an ArrayBuilder. Types must match.
arrow::Status appendFieldValue(arrow::ArrayBuilder* builder,
                                const FieldValue& fv) {
  return std::visit([builder](auto&& v) -> arrow::Status {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, bool>)
      return static_cast<arrow::BooleanBuilder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, int8_t>)
      return static_cast<arrow::Int8Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, int16_t>)
      return static_cast<arrow::Int16Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, int32_t>)
      return static_cast<arrow::Int32Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, int64_t>)
      return static_cast<arrow::Int64Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, uint8_t>)
      return static_cast<arrow::UInt8Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, uint16_t>)
      return static_cast<arrow::UInt16Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, uint32_t>)
      return static_cast<arrow::UInt32Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, uint64_t>)
      return static_cast<arrow::UInt64Builder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, float>)
      return static_cast<arrow::FloatBuilder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, double>)
      return static_cast<arrow::DoubleBuilder*>(builder)->Append(v);
    else if constexpr (std::is_same_v<T, std::string>)
      return static_cast<arrow::StringBuilder*>(builder)->Append(v);
    else
      return builder->AppendNull();
  }, fv);
}

// ---------------------------------------------------------------------------
// Helper: open MCAP reader, read summary, populate DecoderContext, collect
// samples if needed, and prepare the decoder.
// ---------------------------------------------------------------------------
struct McapDecoderSetup {
  std::unique_ptr<MessageDecoder> decoder;
  DecoderContext ctx;

  struct BufferedMessage {
    uint64_t log_time;
    std::vector<std::byte> data;
  };
  std::vector<BufferedMessage> buffered;
};

arrow::Status setupDecoder(const std::string& file_path,
                            const ChannelInfo& channel,
                            mcap::McapReader& reader,
                            McapDecoderSetup& setup) {
  setup.decoder = createDecoder(channel.encoding);
  if (!setup.decoder) {
    return arrow::Status::NotImplemented(
        "Encoding '", channel.encoding, "' not supported. "
        "Enable the corresponding CMake option (e.g. -DWITH_CDR=ON).");
  }

  auto mcap_status = reader.open(file_path);
  if (!mcap_status.ok()) {
    return arrow::Status::IOError("Failed to open MCAP: ",
                                  mcap_status.message);
  }
  reader.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);

  setup.ctx.schema_name = channel.schema_name;
  setup.ctx.schema_encoding = channel.schema_encoding;

  for (const auto& [id, ch_ptr] : reader.channels()) {
    if (ch_ptr->topic == channel.topic) {
      auto schema_ptr = reader.schema(ch_ptr->schemaId);
      if (schema_ptr) {
        setup.ctx.schema_data.assign(
            reinterpret_cast<const std::byte*>(schema_ptr->data.data()),
            reinterpret_cast<const std::byte*>(schema_ptr->data.data()) +
                schema_ptr->data.size());
      }
      break;
    }
  }

  // Collect samples if decoder needs them
  mcap::ReadMessageOptions opts;
  opts.topicFilter = [&](std::string_view topic) {
    return topic == channel.topic;
  };

  if (setup.decoder->needsSamples()) {
    constexpr int kSampleLimit = 100;
    // MCAP scan problems here are non-fatal; successfully-decoded messages
    // still flow. Swallow the warning — any real failure surfaces through
    // the decoder's prepare/decode return status.
    auto messages = reader.readMessages(
        [](const mcap::Status&) {}, opts);

    for (const auto& view : messages) {
      McapDecoderSetup::BufferedMessage bm;
      bm.log_time = view.message.logTime;
      bm.data.assign(
          reinterpret_cast<const std::byte*>(view.message.data),
          reinterpret_cast<const std::byte*>(view.message.data) +
              view.message.dataSize);
      setup.buffered.push_back(std::move(bm));
      if (static_cast<int>(setup.buffered.size()) >= kSampleLimit) break;
    }

    for (const auto& bm : setup.buffered) {
      setup.ctx.samples.push_back(
          {bm.log_time, bm.data.data(),
           static_cast<uint64_t>(bm.data.size())});
    }
  }

  ARROW_RETURN_NOT_OK(setup.decoder->prepare(setup.ctx));
  return arrow::Status::OK();
}

}  // namespace

// ---------------------------------------------------------------------------
// decodeMcapChannel — callback-based message iteration
// ---------------------------------------------------------------------------
arrow::Status decodeMcapChannel(
    const std::string& file_path,
    const ChannelInfo& channel,
    DecodeCallback on_message,
    ProgressCallback progress_cb) {
  mcap::McapReader reader;
  McapDecoderSetup setup;
  ARROW_RETURN_NOT_OK(setupDecoder(file_path, channel, reader, setup));

  mcap::ReadMessageOptions opts;
  opts.topicFilter = [&](std::string_view topic) {
    return topic == channel.topic;
  };

  int64_t row_count = 0;
  int64_t byte_count = 0;

  // Helper lambda: decode a single message and invoke the callback
  auto processMessage = [&](uint64_t log_time, const std::byte* data,
                            uint64_t data_size) -> arrow::Status {
    FieldMap fm;
    auto status = setup.decoder->decode(data, data_size, setup.ctx, fm);
    if (!status.ok()) {
      // Individual bad messages are non-fatal and potentially high-volume;
      // drop silently. Per-topic success/failure is reported by the upload
      // aggregator, and a fully-broken channel surfaces through its status.
      return arrow::Status::OK();
    }
    fm.fields["_log_time_ns"] = log_time;
    ARROW_RETURN_NOT_OK(on_message(fm));

    ++row_count;
    byte_count += static_cast<int64_t>(data_size);
    if (progress_cb) progress_cb(row_count, byte_count, 0);
    return arrow::Status::OK();
  };

  // Process buffered samples first (if needsSamples() was true)
  for (const auto& bm : setup.buffered) {
    ARROW_RETURN_NOT_OK(
        processMessage(bm.log_time, bm.data.data(), bm.data.size()));
  }
  int64_t skip_count = static_cast<int64_t>(setup.buffered.size());
  setup.buffered.clear();

  // Reopen if we consumed the reader during sampling
  if (skip_count > 0) {
    reader.close();
    auto reopen = reader.open(file_path);
    if (!reopen.ok()) {
      return arrow::Status::IOError("Failed to reopen MCAP: ",
                                    reopen.message);
    }
    reader.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan);
  }

  int64_t msg_idx = 0;
  auto all_messages = reader.readMessages(
      [](const mcap::Status&) {}, opts);

  for (const auto& view : all_messages) {
    if (msg_idx++ < skip_count) continue;
    ARROW_RETURN_NOT_OK(processMessage(
        view.message.logTime,
        reinterpret_cast<const std::byte*>(view.message.data),
        view.message.dataSize));
  }
  reader.close();
  return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// indexMcapChannels
// ---------------------------------------------------------------------------
arrow::Status indexMcapChannels(const std::string& file_path,
                                McapChannelIndex& out) {
  mcap::McapReader reader;
  auto status = reader.open(file_path);
  if (!status.ok()) {
    return arrow::Status::IOError("Failed to open MCAP: ", status.message);
  }

  reader.readSummary(mcap::ReadSummaryMethod::AllowFallbackScan,
                     [](const mcap::Status&) {});

  out.file_path = file_path;
  out.file_size_bytes = std::filesystem::file_size(file_path);

  if (reader.statistics()) {
    out.total_messages = reader.statistics()->messageCount;
    out.start_time_ns = reader.statistics()->messageStartTime;
    out.end_time_ns = reader.statistics()->messageEndTime;
  }

  for (const auto& [id, ch_ptr] : reader.channels()) {
    ChannelInfo info;
    info.channel_id = id;
    info.topic = ch_ptr->topic;
    info.encoding = ch_ptr->messageEncoding;

    if (reader.statistics()) {
      auto it = reader.statistics()->channelMessageCounts.find(id);
      if (it != reader.statistics()->channelMessageCounts.end()) {
        info.message_count = it->second;
      }
    }

    auto schema_ptr = reader.schema(ch_ptr->schemaId);
    if (schema_ptr) {
      info.schema_name = schema_ptr->name;
      info.schema_encoding = schema_ptr->encoding;
    }

    out.channels.push_back(std::move(info));
  }

  reader.close();
  return arrow::Status::OK();
}

// ---------------------------------------------------------------------------
// readMcapChannel — uses decodeMcapChannel internally, then builds Arrow
// RecordBatches from the collected FieldMaps.
// ---------------------------------------------------------------------------
arrow::Status readMcapChannel(
    const std::string& file_path, const ChannelInfo& channel,
    std::shared_ptr<arrow::Schema>& out_schema,
    std::vector<std::shared_ptr<arrow::RecordBatch>>& out_batches,
    ProgressCallback progress_cb) {
  // Collect all decoded rows via the callback API
  struct DecodedRow {
    uint64_t log_time;
    FieldMap fields;
  };
  std::vector<DecodedRow> decoded_rows;

  ARROW_RETURN_NOT_OK(decodeMcapChannel(
      file_path, channel,
      [&](const FieldMap& fm) -> arrow::Status {
        DecodedRow row;
        // Extract _log_time_ns and remove it from the field set for columns
        auto it = fm.fields.find("_log_time_ns");
        if (it != fm.fields.end()) {
          row.log_time = std::get<uint64_t>(it->second);
        }
        row.fields = fm;
        row.fields.fields.erase("_log_time_ns");
        decoded_rows.push_back(std::move(row));
        return arrow::Status::OK();
      },
      nullptr));  // We handle progress ourselves below

  if (decoded_rows.empty()) {
    out_schema = arrow::schema({arrow::field("timestamp_ns", arrow::int64())});
    return arrow::Status::OK();
  }

  // Discover schema from all decoded rows
  // Use ordered map to get deterministic column order
  std::map<std::string, std::shared_ptr<arrow::DataType>> col_types;
  for (const auto& row : decoded_rows) {
    for (const auto& [key, val] : row.fields.fields) {
      if (col_types.find(key) == col_types.end()) {
        col_types[key] = arrowTypeFromFieldValue(val);
      }
    }
  }

  // Build schema: timestamp_ns first, then all discovered columns
  arrow::FieldVector fields;
  fields.push_back(arrow::field("timestamp_ns", arrow::int64()));
  std::vector<std::string> col_order;
  for (const auto& [name, type] : col_types) {
    fields.push_back(arrow::field(name, type, /*nullable=*/true));
    col_order.push_back(name);
  }
  out_schema = arrow::schema(fields);

  // Build batches
  int64_t total_rows = 0;
  int64_t total_bytes = 0;

  auto createBuilders = [&]() {
    std::vector<std::unique_ptr<arrow::ArrayBuilder>> builders;
    for (int i = 0; i < out_schema->num_fields(); ++i) {
      std::unique_ptr<arrow::ArrayBuilder> b;
      arrow::MakeBuilder(arrow::default_memory_pool(),
                         out_schema->field(i)->type(), &b);
      builders.push_back(std::move(b));
    }
    return builders;
  };

  auto builders = createBuilders();
  int64_t rows_in_batch = 0;

  auto flushBatch = [&]() -> arrow::Status {
    if (rows_in_batch == 0) return arrow::Status::OK();
    arrow::ArrayVector arrays;
    arrays.reserve(builders.size());
    for (auto& b : builders) {
      std::shared_ptr<arrow::Array> arr;
      ARROW_RETURN_NOT_OK(b->Finish(&arr));
      arrays.push_back(std::move(arr));
    }
    out_batches.push_back(
        arrow::RecordBatch::Make(out_schema, rows_in_batch, arrays));
    builders = createBuilders();
    rows_in_batch = 0;
    return arrow::Status::OK();
  };

  for (const auto& row : decoded_rows) {
    // Append timestamp
    ARROW_RETURN_NOT_OK(
        static_cast<arrow::Int64Builder*>(builders[0].get())
            ->Append(static_cast<int64_t>(row.log_time)));

    // Append each column
    for (size_t c = 0; c < col_order.size(); ++c) {
      auto it = row.fields.fields.find(col_order[c]);
      if (it != row.fields.fields.end()) {
        ARROW_RETURN_NOT_OK(appendFieldValue(builders[c + 1].get(), it->second));
      } else {
        ARROW_RETURN_NOT_OK(builders[c + 1]->AppendNull());
      }
    }
    ++rows_in_batch;
    ++total_rows;
    if (rows_in_batch >= kBatchSize) {
      ARROW_RETURN_NOT_OK(flushBatch());
      if (progress_cb) progress_cb(total_rows, total_bytes, 0);
    }
  }

  if (rows_in_batch > 0) {
    ARROW_RETURN_NOT_OK(flushBatch());
    if (progress_cb) progress_cb(total_rows, total_bytes, 0);
  }

  return arrow::Status::OK();
}

}  // namespace mosaico
