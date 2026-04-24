#include "harness/server_fixture.hpp"

namespace mosaico::test {

class Version : public ServerFixture {};

TEST_F(Version, ReturnsValidSemver) {
    auto client = makeAdminClient();
    auto r = client->version();
    ASSERT_TRUE(r.ok()) << r.status().ToString();

    // Version string is non-empty and looks like "X.Y.Z" or "X.Y.Z-pre".
    EXPECT_FALSE(r->version.empty());
    // semver components must round-trip with the string.
    std::string rebuilt = std::to_string(r->major) + "." +
                          std::to_string(r->minor) + "." +
                          std::to_string(r->patch);
    if (!r->pre.empty()) rebuilt += "-" + r->pre;
    EXPECT_EQ(r->version, rebuilt)
        << "version string '" << r->version
        << "' does not match assembled semver '" << rebuilt << "'";
}

}  // namespace mosaico::test
