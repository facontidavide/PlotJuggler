#include "harness/server_fixture.hpp"
#include "harness/timing.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>

#include <algorithm>
#include <memory>
#include <unistd.h>

namespace mosaico::test {

class SequenceLifecycle : public ServerFixture {};

namespace {

// Mosaico message envelope: per Python SDK models/message.py, every batch
// needs `timestamp_ns` (int64, non-null) and may include optional
// recording_timestamp_ns, frame_id, sequence_id, plus ontology payload.
std::shared_ptr<arrow::RecordBatch> makeTinyBatch() {
    arrow::Int64Builder ts_builder;
    arrow::DoubleBuilder val_builder;
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(ts_builder.Append(static_cast<int64_t>(i) * 1'000'000'000LL).ok());
        EXPECT_TRUE(val_builder.Append(static_cast<double>(i) * 0.5).ok());
    }
    std::shared_ptr<arrow::Array> ts_array, val_array;
    EXPECT_TRUE(ts_builder.Finish(&ts_array).ok());
    EXPECT_TRUE(val_builder.Finish(&val_array).ok());

    auto schema = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64(), /*nullable=*/false),
        arrow::field("value", arrow::float64()),
    });
    return arrow::RecordBatch::Make(schema, 10, {ts_array, val_array});
}

std::string uniqueSequenceName() {
    return "lifecycle_" + std::to_string(::getpid()) + "_" +
           std::to_string(std::rand());
}

}  // namespace

// Phase-1 smoke: create a sequence, verify it lists, delete, verify gone.
// Exercises the whole wire: TLS handshake, auth, DoAction(sequence_create),
// DoPut (upload one batch), DoAction(session_finalize), ListFlights,
// DoAction(sequence_delete).
TEST_F(SequenceLifecycle, CreateUploadListDelete) {
    TimingWaterfall w("sequence_lifecycle");

    auto client = makeAdminClient();
    w.mark("client_constructed");

    const std::string seq_name = uniqueSequenceName();

    // Upload one tiny topic.
    auto session_r = client->beginUpload(seq_name);
    ASSERT_TRUE(session_r.ok()) << session_r.status().ToString();
    auto session = std::move(session_r).ValueOrDie();
    w.mark("session_begun");

    auto batch = makeTinyBatch();
    ASSERT_TRUE(session->createTopic("smoke_topic", "raw", batch->schema()).ok());
    ASSERT_TRUE(session->putBatch(batch).ok());
    ASSERT_TRUE(session->closeTopic().ok());
    ASSERT_TRUE(session->finalize().ok());
    w.mark("upload_finalized");

    // List — our sequence must appear.
    auto list_r = client->listSequences();
    ASSERT_TRUE(list_r.ok()) << list_r.status().ToString();
    const auto& seqs = *list_r;
    auto found = std::find_if(seqs.begin(), seqs.end(),
        [&](const SequenceInfo& s) { return s.name == seq_name; });
    ASSERT_NE(found, seqs.end()) << "uploaded sequence not in list";
    w.mark("listed");

    // Delete.
    auto del = client->deleteSequence(seq_name);
    ASSERT_TRUE(del.ok()) << del.ToString();
    w.mark("deleted");

    // Verify gone.
    auto list2_r = client->listSequences();
    ASSERT_TRUE(list2_r.ok()) << list2_r.status().ToString();
    const auto& seqs2 = *list2_r;
    auto still_there = std::find_if(seqs2.begin(), seqs2.end(),
        [&](const SequenceInfo& s) { return s.name == seq_name; });
    EXPECT_EQ(still_there, seqs2.end()) << "sequence still listed after delete";
    w.mark("list_after_delete");

    w.emit("sequence_lifecycle_timing.json");
}

}  // namespace mosaico::test
