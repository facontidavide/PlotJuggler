#include "flight/upload_mcap.hpp"

#include "mcap_reader.hpp"
#include "ontology/ontology_registry.hpp"
#include "ontology/tag_resolver.hpp"

#include <arrow/api.h>

#include <sstream>

namespace mosaico {

namespace {

// Build one aggregated status message summarizing per-topic failures.
// At most the first 5 failed topics are named; the rest are collapsed
// into a count so the message stays UI-friendly.
arrow::Status makeAggregateFailure(const UploadReport& report,
                                   const arrow::Status& finalize_status)
{
    std::ostringstream msg;
    const size_t failed = report.topic_failures.size();
    msg << failed << " of " << report.topics_attempted
        << " topics failed to upload";

    const size_t shown = std::min<size_t>(failed, 5);
    for (size_t i = 0; i < shown; ++i) {
        const auto& [topic, err] = report.topic_failures[i];
        msg << "\n  - " << topic << ": " << err;
    }
    if (failed > shown) {
        msg << "\n  - (+ " << (failed - shown) << " more)";
    }
    if (!finalize_status.ok()) {
        msg << "\nfinalize: " << finalize_status.message();
    }
    return arrow::Status::IOError(msg.str());
}

}  // namespace

arrow::Status uploadMcapImpl(MosaicoClient& client,
                             const std::string& mcap_path,
                             const std::string& sequence_name,
                             UploadProgressCallback progress_cb,
                             std::atomic<bool>* interrupted,
                             UploadReport& out);

arrow::Status uploadMcap(MosaicoClient& client,
                         const std::string& mcap_path,
                         const std::string& sequence_name,
                         UploadProgressCallback progress_cb,
                         std::atomic<bool>* interrupted,
                         UploadReport* report)
{
  // MCAP parsing (mcap::McapReader) and std::filesystem can throw on
  // truncated/inaccessible files. Convert every exception to a Status so
  // the worker thread never unwinds into PlotJuggler's event loop.
  UploadReport local_report;
  UploadReport& out = report ? *report : local_report;
  try {
    return uploadMcapImpl(client, mcap_path, sequence_name,
                          std::move(progress_cb), interrupted, out);
  } catch (const std::exception& e) {
    return arrow::Status::IOError("MCAP upload failed: ", e.what());
  } catch (...) {
    return arrow::Status::IOError("MCAP upload failed: unknown error");
  }
}

arrow::Status uploadMcapImpl(MosaicoClient& client,
                             const std::string& mcap_path,
                             const std::string& sequence_name,
                             UploadProgressCallback progress_cb,
                             std::atomic<bool>* interrupted,
                             UploadReport& out)
{
  auto is_interrupted = [&]() {
    return interrupted && interrupted->load(std::memory_order_relaxed);
  };

  McapChannelIndex index;
  ARROW_RETURN_NOT_OK(indexMcapChannels(mcap_path, index));

  auto resolver = createRosTagResolver();
  OntologyRegistry registry;

  ARROW_ASSIGN_OR_RAISE(auto upload, client.beginUpload(sequence_name));

  auto record_failure = [&](const std::string& topic,
                            const arrow::Status& status) {
      out.topic_failures.emplace_back(topic, status.ToString());
  };

  for (const auto& channel : index.channels) {
    if (is_interrupted()) break;

    auto ontology_tag = resolver.resolve(channel.schema_name);
    OntologyBuilder* builder = nullptr;
    if (ontology_tag) builder = registry.find(*ontology_tag);
    if (!builder) continue;  // Channel has no adapter — not an upload target.

    out.topics_attempted++;

    auto status = upload->createTopic(channel.topic, builder->ontologyTag(),
                                      builder->schema());
    if (!status.ok()) {
      record_failure(channel.topic, status);
      continue;
    }

    status = decodeMcapChannel(
        mcap_path, channel,
        [&](const FieldMap& fields) -> arrow::Status {
          if (is_interrupted())
            return arrow::Status::Cancelled("interrupted");
          ARROW_RETURN_NOT_OK(builder->append(fields));
          if (builder->shouldFlush()) {
            ARROW_ASSIGN_OR_RAISE(auto batch, builder->flush());
            if (batch) ARROW_RETURN_NOT_OK(upload->putBatch(batch));
          }
          return arrow::Status::OK();
        },
        [&](int64_t rows, int64_t bytes, int64_t /*total_bytes*/) {
          if (progress_cb) {
            progress_cb(rows, bytes,
                        static_cast<int64_t>(channel.message_count),
                        channel.topic);
          }
        });

    if (!status.ok()) {
      record_failure(channel.topic, status);
      (void)upload->closeTopic();
      continue;
    }

    auto batch_result = builder->flush();
    if (!batch_result.ok()) {
      record_failure(channel.topic, batch_result.status());
      (void)upload->closeTopic();
      continue;
    }
    if (*batch_result) {
      auto put_status = upload->putBatch(*batch_result);
      if (!put_status.ok()) {
        record_failure(channel.topic, put_status);
        (void)upload->closeTopic();
        continue;
      }
    }

    (void)upload->closeTopic();
    out.topics_succeeded++;
  }

  if (is_interrupted()) {
    (void)upload->cleanup();
    return arrow::Status::Cancelled("upload cancelled");
  }

  auto finalize_status = upload->finalize();

  if (!out.topic_failures.empty()) {
    return makeAggregateFailure(out, finalize_status);
  }
  return finalize_status;
}

}  // namespace mosaico
