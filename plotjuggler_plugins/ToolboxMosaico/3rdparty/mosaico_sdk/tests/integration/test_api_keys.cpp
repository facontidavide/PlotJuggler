#include "harness/server_fixture.hpp"

namespace mosaico::test {

class ApiKeys : public ServerFixture {};

// Happy path: create a read-only key, query its status, revoke, confirm
// subsequent usage fails.
TEST_F(ApiKeys, CreateStatusRevoke) {
    auto admin = makeAdminClient();

    auto created = admin->createApiKey(
        ApiKeyPermission::Read, "integration-test-read-only");
    ASSERT_TRUE(created.ok()) << created.status().ToString();
    ASSERT_FALSE(created->api_key_token.empty());
    ASSERT_FALSE(created->api_key_fingerprint.empty());

    auto status = admin->apiKeyStatus(created->api_key_fingerprint);
    ASSERT_TRUE(status.ok()) << status.status().ToString();
    EXPECT_EQ(status->api_key_fingerprint, created->api_key_fingerprint);
    EXPECT_EQ(status->description, "integration-test-read-only");
    EXPECT_GT(status->created_at_ns, 0);

    ASSERT_TRUE(admin->revokeApiKey(created->api_key_fingerprint).ok());

    // After revocation, the key fingerprint should no longer be queryable.
    auto after = admin->apiKeyStatus(created->api_key_fingerprint);
    EXPECT_FALSE(after.ok()) << "revoked key should not report status";
}

// Non-manage keys cannot create new keys (permission tier enforcement).
TEST_F(ApiKeys, NonManageCannotCreate) {
    auto admin = makeAdminClient();

    auto writer = admin->createApiKey(ApiKeyPermission::Write, "writer-only");
    ASSERT_TRUE(writer.ok()) << writer.status().ToString();

    // Build a client using the write-tier token.
    MosaicoClient writer_client(uri(), /*timeout=*/10, /*pool=*/1,
                                caCertPath(), writer->api_key_token);

    auto attempt = writer_client.createApiKey(ApiKeyPermission::Read, "from-writer");
    EXPECT_FALSE(attempt.ok()) << "write-tier key should not create keys";

    // Cleanup the writer key itself (admin scope).
    (void)admin->revokeApiKey(writer->api_key_fingerprint);
}

}  // namespace mosaico::test
