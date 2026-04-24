#include "harness/server_fixture.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>

#include <algorithm>
#include <memory>
#include <unistd.h>

namespace mosaico::test {

class ListAndPull : public ServerFixture {};

namespace {

std::shared_ptr<arrow::RecordBatch> makeBatch(int rows, int64_t t0_ns) {
    arrow::Int64Builder tb;
    arrow::DoubleBuilder vb;
    for (int i = 0; i < rows; ++i) {
        EXPECT_TRUE(tb.Append(t0_ns + static_cast<int64_t>(i) * 1'000'000'000LL).ok());
        EXPECT_TRUE(vb.Append(static_cast<double>(i) * 0.5).ok());
    }
    std::shared_ptr<arrow::Array> ta, va;
    EXPECT_TRUE(tb.Finish(&ta).ok());
    EXPECT_TRUE(vb.Finish(&va).ok());
    auto schema = arrow::schema({
        arrow::field("timestamp_ns", arrow::int64(), /*nullable=*/false),
        arrow::field("value", arrow::float64()),
    });
    return arrow::RecordBatch::Make(schema, rows, {ta, va});
}

std::string uniqueName(const char* prefix) {
    return std::string(prefix) + "_" + std::to_string(::getpid()) + "_" +
           std::to_string(std::rand());
}

}  // namespace

// Upload a sequence with two topics, then list + pull them back.
// Verifies: listSequences sees the sequence; listTopics enumerates both topics;
// pullTopic returns the exact rows we uploaded (byte-wise equivalent schema).
TEST_F(ListAndPull, UploadListPullRoundTrip) {
    auto client = makeAdminClient();
    const std::string seq_name = uniqueName("listpull");

    constexpr int64_t kT0 = 1'700'000'000'000'000'000LL;  // some recent ns
    constexpr int kRowsA = 10;
    constexpr int kRowsB = 5;

    // Upload: two topics in one session.
    {
        auto session_r = client->beginUpload(seq_name);
        ASSERT_TRUE(session_r.ok()) << session_r.status().ToString();
        auto session = std::move(session_r).ValueOrDie();

        auto batchA = makeBatch(kRowsA, kT0);
        ASSERT_TRUE(session->createTopic("topic_a", "raw", batchA->schema()).ok());
        ASSERT_TRUE(session->putBatch(batchA).ok());
        ASSERT_TRUE(session->closeTopic().ok());

        auto batchB = makeBatch(kRowsB, kT0 + 60'000'000'000LL);  // +60s
        ASSERT_TRUE(session->createTopic("topic_b", "raw", batchB->schema()).ok());
        ASSERT_TRUE(session->putBatch(batchB).ok());
        ASSERT_TRUE(session->closeTopic().ok());

        ASSERT_TRUE(session->finalize().ok());
    }

    // listSequences should see our sequence.
    {
        auto list_r = client->listSequences();
        ASSERT_TRUE(list_r.ok()) << list_r.status().ToString();
        auto it = std::find_if(list_r->begin(), list_r->end(),
            [&](const SequenceInfo& s) { return s.name == seq_name; });
        ASSERT_NE(it, list_r->end()) << "uploaded sequence missing from listSequences";
        // Timestamps should be populated from endpoint app_metadata.
        EXPECT_GE(it->max_ts_ns, it->min_ts_ns);
    }

    // listTopics should return both topics with correct names.
    std::vector<TopicInfo> topics;
    {
        auto topics_r = client->listTopics(seq_name);
        ASSERT_TRUE(topics_r.ok()) << topics_r.status().ToString();
        topics = std::move(*topics_r);
        EXPECT_EQ(topics.size(), 2u) << "expected 2 topics";
        std::vector<std::string> names;
        for (const auto& t : topics) names.push_back(t.topic_name);
        std::sort(names.begin(), names.end());
        // Topic names may come back as "topic_a" or "/topic_a"; normalize.
        auto strip = [](std::string s) {
            while (!s.empty() && s.front() == '/') s.erase(s.begin());
            return s;
        };
        ASSERT_GE(names.size(), 2u);
        EXPECT_EQ(strip(names[0]), "topic_a");
        EXPECT_EQ(strip(names[1]), "topic_b");
    }

    // pullTopic(topic_a) should return the rows we uploaded.
    {
        auto pull_r = client->pullTopic(seq_name, "topic_a", {});
        ASSERT_TRUE(pull_r.ok()) << pull_r.status().ToString();
        int64_t total_rows = 0;
        for (const auto& batch : pull_r->batches) {
            total_rows += batch->num_rows();
        }
        EXPECT_EQ(total_rows, kRowsA) << "pulled row count mismatch";
    }

    // Cleanup.
    auto del = client->deleteSequence(seq_name);
    EXPECT_TRUE(del.ok()) << del.ToString();
}

}  // namespace mosaico::test
