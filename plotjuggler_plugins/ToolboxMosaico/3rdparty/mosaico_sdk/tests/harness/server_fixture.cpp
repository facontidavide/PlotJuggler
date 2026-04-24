#include "server_fixture.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace mosaico::test {

std::string ServerFixture::uri_;
std::string ServerFixture::ca_cert_path_;
std::string ServerFixture::admin_key_;
std::string ServerFixture::compose_dir_;
bool ServerFixture::booted_ = false;

namespace {

// The script lives next to this file. MOSAICO_HARNESS_SCRIPT is injected as
// a CMake compile definition so tests find it regardless of cwd.
#ifndef MOSAICO_HARNESS_SCRIPT
#error "MOSAICO_HARNESS_SCRIPT must be defined by the build (absolute path to setup_local_mosaico.sh)"
#endif

std::string shell(const std::string& cmd) {
    std::array<char, 4096> buf{};
    std::string out;
    FILE* pipe = ::popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen failed: " + cmd);
    }
    while (std::fgets(buf.data(), buf.size(), pipe) != nullptr) {
        out.append(buf.data());
    }
    int rc = ::pclose(pipe);
    if (rc != 0) {
        throw std::runtime_error("command failed (rc=" + std::to_string(rc) +
                                 "):\n  " + cmd + "\noutput:\n" + out);
    }
    return out;
}

bool envFlag(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && *v != '\0' && std::string(v) != "0";
}

}  // namespace

// ---------------------------------------------------------------------------

std::string ServerFixture::runHarness(const std::string& args) {
    std::string cmd = std::string("bash ") + MOSAICO_HARNESS_SCRIPT + " " + args + " 2>&1";
    return shell(cmd);
}

void ServerFixture::parseSummary(const std::string& text) {
    // The script prints:
    //   Server URI:      grpc+tls://localhost:6726
    //   Client CA cert:  /tmp/mosaico-certs/ca.pem
    //   Admin API key:   msco_<32>_<8>
    std::regex uri_re(R"(Server URI:\s*(\S+))");
    std::regex ca_re(R"(Client CA cert:\s*(\S+))");
    std::regex key_re(R"(Admin API key:\s*(msco_[a-z0-9]{32}_[0-9a-f]{8}))");
    std::smatch m;

    if (std::regex_search(text, m, uri_re))  uri_ = m[1].str();
    if (std::regex_search(text, m, ca_re))   ca_cert_path_ = m[1].str();
    if (std::regex_search(text, m, key_re))  admin_key_ = m[1].str();

    // Compose dir lives at /tmp/mosaico-test by default; script prints it in
    // the "Manage:" block as `docker compose -f <dir>/compose.yml`.
    std::regex cd_re(R"(docker compose -f (\S+)/compose\.yml)");
    if (std::regex_search(text, m, cd_re)) compose_dir_ = m[1].str();

    if (uri_.empty() || ca_cert_path_.empty() || admin_key_.empty()) {
        throw std::runtime_error("harness summary missing fields. Full output:\n" + text);
    }
}

void ServerFixture::SetUpTestSuite() {
    if (booted_) return;  // defensive — GTest already guards this

    std::string args = "--reuse-stack --keep-certs";
    if (const char* extra = std::getenv("MOSAICO_HARNESS_ARGS")) {
        args += " ";
        args += extra;
    }
    try {
        auto output = runHarness(args);
        parseSummary(output);
        booted_ = true;
    } catch (...) {
        // Orphan containers from a half-successful bringup are worse than a
        // noisy failure. Run cleanup on the way out, then rethrow.
        try { (void)runHarness("cleanup"); } catch (...) {}
        throw;
    }
}

void ServerFixture::TearDownTestSuite() {
    if (!booted_) return;
    if (envFlag("MOSAICO_KEEP_STACK")) {
        std::fprintf(stderr, "[ServerFixture] MOSAICO_KEEP_STACK set — leaving stack up at %s\n",
                     uri_.c_str());
        return;
    }
    try {
        (void)runHarness("cleanup");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[ServerFixture] cleanup failed: %s\n", e.what());
    }
    booted_ = false;
    uri_.clear();
    ca_cert_path_.clear();
    admin_key_.clear();
    compose_dir_.clear();
}

std::unique_ptr<MosaicoClient> ServerFixture::makeAdminClient(int timeout_seconds,
                                                              size_t pool_size) {
    return std::make_unique<MosaicoClient>(uri_, timeout_seconds, pool_size,
                                           ca_cert_path_, admin_key_);
}

ServerFixture::KeyHandle ServerFixture::makeKey(const std::string& permission) {
    // Ask mosaicod (inside the container) to issue a fresh key.
    std::string desc = "test-key-" + permission + "-" +
                       std::to_string(::getpid()) + "-" +
                       std::to_string(std::rand());
    std::string cmd = "docker compose -f " + compose_dir_ +
                      "/compose.yml exec -T mosaicod mosaicod api-key create"
                      " --permissions " + permission +
                      " -d \"" + desc + "\" 2>&1";
    std::string out = shell(cmd);

    std::regex key_re(R"((msco_[a-z0-9]{32}_[0-9a-f]{8}))");
    std::smatch m;
    if (!std::regex_search(out, m, key_re)) {
        throw std::runtime_error("failed to extract key from output:\n" + out);
    }
    return KeyHandle(m[1].str(), std::move(desc));
}

// KeyHandle -----------------------------------------------------------------

ServerFixture::KeyHandle::KeyHandle(std::string token, std::string description)
    : token_(std::move(token)), description_(std::move(description)) {}

ServerFixture::KeyHandle::KeyHandle(KeyHandle&& o) noexcept
    : token_(std::move(o.token_)), description_(std::move(o.description_)) {
    o.token_.clear();
}

ServerFixture::KeyHandle& ServerFixture::KeyHandle::operator=(KeyHandle&& o) noexcept {
    if (this != &o) {
        token_ = std::move(o.token_);
        description_ = std::move(o.description_);
        o.token_.clear();
    }
    return *this;
}

ServerFixture::KeyHandle::~KeyHandle() {
    if (token_.empty()) return;
    // Best-effort revoke — never throw from destructor.
    try {
        std::string cmd = "docker compose -f " + ServerFixture::compose_dir_ +
                          "/compose.yml exec -T mosaicod mosaicod api-key revoke "
                          "--token " + token_ + " >/dev/null 2>&1 || true";
        (void)shell(cmd);
    } catch (...) {
        // swallow — best effort
    }
}

// Fault injection -----------------------------------------------------------

void ServerFixture::pauseServer() {
    (void)shell("docker compose -f " + compose_dir_ + "/compose.yml pause mosaicod");
}
void ServerFixture::resumeServer() {
    (void)shell("docker compose -f " + compose_dir_ + "/compose.yml unpause mosaicod");
}
void ServerFixture::killServer() {
    (void)shell("docker compose -f " + compose_dir_ + "/compose.yml kill mosaicod");
}

}  // namespace mosaico::test
