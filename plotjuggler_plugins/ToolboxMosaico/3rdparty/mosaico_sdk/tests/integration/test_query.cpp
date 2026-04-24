#include "harness/server_fixture.hpp"

#include <arrow/api.h>
#include <arrow/builder.h>

#include <algorithm>
#include <unistd.h>

namespace mosaico::test {

class Query : public ServerFixture {};

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

// Upload a sequence with one topic. Returns the sequence name.
std::string uploadOne(MosaicoClient& c, const std::string& prefix,
                      const std::string& topic, const std::string& ontology) {
    const std::string seq = prefix + "_" + std::to_string(std::rand());
    auto s = std::move(c.beginUpload(seq)).ValueOrDie();
    EXPECT_TRUE(s->createTopic(topic, ontology, tinySchema()).ok());
    EXPECT_TRUE(s->closeTopic().ok());
    EXPECT_TRUE(s->finalize().ok());
    return seq;
}

bool contains(const QueryResponse& r, const std::string& seq) {
    return std::any_of(r.items.begin(), r.items.end(),
                       [&](const QueryResponseItem& i) { return i.sequence == seq; });
}

}  // namespace

// Query by sequence name (exact) finds the exact match and nothing else.
TEST_F(Query, BySequenceNameExact) {
    auto client = makeAdminClient();
    const std::string a = uploadOne(*client, uniq("q_a"), "t", "raw");
    const std::string b = uploadOne(*client, uniq("q_b"), "t", "raw");

    auto r = client->query({QuerySequenceBuilder().withName(a).build()});
    ASSERT_TRUE(r.ok()) << r.status().ToString();

    EXPECT_TRUE(contains(*r, a));
    EXPECT_FALSE(contains(*r, b));

    (void)client->deleteSequence(a);
    (void)client->deleteSequence(b);
}

// Query by sequence name partial match finds only matching prefix.
TEST_F(Query, BySequenceNameMatch) {
    auto client = makeAdminClient();
    const std::string prefix = uniq("qmatch");
    const std::string a = uploadOne(*client, prefix + "_foo", "t", "raw");
    const std::string b = uploadOne(*client, uniq("other"), "t", "raw");

    auto r = client->query({QuerySequenceBuilder().withNameMatch(prefix).build()});
    ASSERT_TRUE(r.ok()) << r.status().ToString();

    EXPECT_TRUE(contains(*r, a)) << "prefix match should hit " << a;
    EXPECT_FALSE(contains(*r, b)) << "unrelated sequence should not match";

    (void)client->deleteSequence(a);
    (void)client->deleteSequence(b);
}

// Query by topic ontology_tag filters to topics with that tag.
TEST_F(Query, ByTopicOntology) {
    auto client = makeAdminClient();
    const std::string seq_imu = uploadOne(*client, uniq("qimu"), "sensor", "imu");
    const std::string seq_raw = uploadOne(*client, uniq("qraw"), "sensor", "raw");

    auto r = client->query({QueryTopicBuilder().withOntologyTag("imu").build()});
    ASSERT_TRUE(r.ok()) << r.status().ToString();

    EXPECT_TRUE(contains(*r, seq_imu));
    EXPECT_FALSE(contains(*r, seq_raw)) << "raw-ontology sequence must not match imu filter";

    (void)client->deleteSequence(seq_imu);
    (void)client->deleteSequence(seq_raw);
}

}  // namespace mosaico::test
