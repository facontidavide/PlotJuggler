#pragma once

#include "flight/mosaico_client.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace mosaico::test {

// One running mosaicod container shared across every test in a suite.
// Call sites inherit this as a GTest fixture; SetUpTestSuite boots the
// container via tests/harness/setup_local_mosaico.sh and parses the summary
// block for URI / CA cert path / admin API key.
//
// Cleanup is skipped when MOSAICO_KEEP_STACK is set in the environment —
// use that for the tight dev loop (rebuild tests, re-run in 2s instead of
// 30s restart churn).
class ServerFixture : public ::testing::Test {
 public:
    static void SetUpTestSuite();
    static void TearDownTestSuite();

    // The booted server.
    static const std::string& uri() { return uri_; }
    static const std::string& caCertPath() { return ca_cert_path_; }
    static const std::string& adminKey() { return admin_key_; }

    // Convenience: build a client with admin permissions.
    static std::unique_ptr<MosaicoClient> makeAdminClient(int timeout_seconds = 10,
                                                         size_t pool_size = 2);

    // Create a scoped API key with the given permissions. Permissions is
    // one of: "read", "write", "delete", "manage". Returns the msco_ token.
    // Key survives only until cleanup() is called on the returned handle.
    class KeyHandle {
     public:
        KeyHandle(std::string token, std::string description);
        KeyHandle(const KeyHandle&) = delete;
        KeyHandle& operator=(const KeyHandle&) = delete;
        KeyHandle(KeyHandle&&) noexcept;
        KeyHandle& operator=(KeyHandle&&) noexcept;
        ~KeyHandle();

        const std::string& token() const { return token_; }

     private:
        std::string token_;
        std::string description_;
    };
    static KeyHandle makeKey(const std::string& permission);

    // Fault-injection helpers (used by stress tests, exposed here for reuse).
    static void pauseServer();
    static void resumeServer();
    static void killServer();

 private:
    static std::string runHarness(const std::string& args);
    static void parseSummary(const std::string& stdout_text);

    static std::string uri_;
    static std::string ca_cert_path_;
    static std::string admin_key_;
    static std::string compose_dir_;  // for cleanup + docker exec
    static bool booted_;
};

}  // namespace mosaico::test
