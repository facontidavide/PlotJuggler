#include "harness/server_fixture.hpp"

#include <arrow/api.h>
#include <unistd.h>

#include <algorithm>

namespace mosaico::test {

class Notifications : public ServerFixture {};

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

void uploadMinimal(MosaicoClient& c, const std::string& seq,
                   const std::string& topic = "t") {
    auto s = std::move(c.beginUpload(seq)).ValueOrDie();
    EXPECT_TRUE(s->createTopic(topic, "raw", tinySchema()).ok());
    EXPECT_TRUE(s->closeTopic().ok());
    EXPECT_TRUE(s->finalize().ok());
}

}  // namespace

TEST_F(Notifications, SequenceReportListPurge) {
    auto client = makeAdminClient();
    const std::string seq = uniq("notifseq");
    uploadMinimal(*client, seq);

    // Empty initially.
    auto empty_r = client->listSequenceNotifications(seq);
    ASSERT_TRUE(empty_r.ok()) << empty_r.status().ToString();
    EXPECT_EQ(empty_r->size(), 0u);

    // Post two error notifications.
    ASSERT_TRUE(client->reportSequenceNotification(seq, "error", "first").ok());
    ASSERT_TRUE(client->reportSequenceNotification(seq, "error", "second").ok());

    auto two_r = client->listSequenceNotifications(seq);
    ASSERT_TRUE(two_r.ok()) << two_r.status().ToString();
    ASSERT_EQ(two_r->size(), 2u);

    bool saw_first = false, saw_second = false;
    for (const auto& n : *two_r) {
        EXPECT_EQ(n.type, "error");
        if (n.msg == "first") saw_first = true;
        if (n.msg == "second") saw_second = true;
        EXPECT_FALSE(n.created_datetime.empty());
    }
    EXPECT_TRUE(saw_first);
    EXPECT_TRUE(saw_second);

    // Purge and confirm empty.
    ASSERT_TRUE(client->purgeSequenceNotifications(seq).ok());
    auto after_r = client->listSequenceNotifications(seq);
    ASSERT_TRUE(after_r.ok()) << after_r.status().ToString();
    EXPECT_EQ(after_r->size(), 0u);

    (void)client->deleteSequence(seq);
}

TEST_F(Notifications, TopicReportListPurge) {
    auto client = makeAdminClient();
    const std::string seq = uniq("notiftop");
    const std::string topic = "t";
    uploadMinimal(*client, seq, topic);

    auto empty_r = client->listTopicNotifications(seq, topic);
    ASSERT_TRUE(empty_r.ok()) << empty_r.status().ToString();
    EXPECT_EQ(empty_r->size(), 0u);

    ASSERT_TRUE(client->reportTopicNotification(seq, topic, "error", "boom").ok());

    auto one_r = client->listTopicNotifications(seq, topic);
    ASSERT_TRUE(one_r.ok()) << one_r.status().ToString();
    ASSERT_EQ(one_r->size(), 1u);
    EXPECT_EQ((*one_r)[0].type, "error");
    EXPECT_EQ((*one_r)[0].msg, "boom");

    ASSERT_TRUE(client->purgeTopicNotifications(seq, topic).ok());
    auto after_r = client->listTopicNotifications(seq, topic);
    ASSERT_TRUE(after_r.ok()) << after_r.status().ToString();
    EXPECT_EQ(after_r->size(), 0u);

    (void)client->deleteSequence(seq);
}

}  // namespace mosaico::test
