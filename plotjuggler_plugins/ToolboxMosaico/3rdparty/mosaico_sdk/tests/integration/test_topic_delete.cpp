#include "harness/server_fixture.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>

#include <unistd.h>

namespace mosaico::test {

// topic_delete is an error-path helper in the Mosaico protocol (see the
// Python SDK — it's invoked only when topic_create fails mid-session to roll
// back an orphaned topic). These tests pin the behavioral contract:
//   1. The wire call succeeds against a live topic in an active session.
//   2. Against a finalized (locked) topic, the server rejects with "locked".
class TopicDelete : public ServerFixture {};

namespace {

std::shared_ptr<arrow::Schema> tinySchema() {
    return arrow::schema({
        arrow::field("timestamp_ns", arrow::int64(), /*nullable=*/false),
        arrow::field("value", arrow::float64()),
    });
}

std::string uniq(const char* p) {
    return std::string(p) + "_" + std::to_string(::getpid()) + "_" +
           std::to_string(std::rand());
}

}  // namespace

TEST_F(TopicDelete, NonexistentTopicFails) {
    auto client = makeAdminClient();
    auto del = client->deleteTopic("no_such_sequence_xyz", "no_such_topic");
    EXPECT_FALSE(del.ok()) << "expected delete of nonexistent topic to fail";
    // Error is from the server — our payload reached it and was rejected
    // with a real resource-not-found / permission error, not a parse error.
    EXPECT_EQ(del.ToString().find("deserialization"), std::string::npos)
        << "payload shape should be accepted: " << del.ToString();
}

TEST_F(TopicDelete, LockedWhenFinalized) {
    auto client = makeAdminClient();
    const std::string seq = uniq("topicdel_locked");

    {
        auto s = std::move(client->beginUpload(seq)).ValueOrDie();
        ASSERT_TRUE(s->createTopic("locked", "raw", tinySchema()).ok());
        ASSERT_TRUE(s->closeTopic().ok());
        ASSERT_TRUE(s->finalize().ok());
    }

    auto del = client->deleteTopic(seq, "locked");
    EXPECT_FALSE(del.ok()) << "expected delete of finalized topic to fail";
    EXPECT_NE(del.ToString().find("lock"), std::string::npos)
        << "expected 'lock' in error: " << del.ToString();

    (void)client->deleteSequence(seq);
}

}  // namespace mosaico::test
