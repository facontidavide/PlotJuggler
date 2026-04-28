// src/flight/connection_pool.cpp
#include "flight/connection_pool.hpp"

#include "flight/logging.hpp"

#include <arrow/flight/api.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace fl = arrow::flight;

namespace mosaico {

namespace {

// Middleware that adds the Mosaico API key header to every outbound call.
class ApiKeyMiddleware : public fl::ClientMiddleware {
 public:
    explicit ApiKeyMiddleware(std::string api_key)
        : api_key_(std::move(api_key)) {}

    void SendingHeaders(fl::AddCallHeaders* outgoing_headers) override {
        outgoing_headers->AddHeader("mosaico-api-key-token", api_key_);
    }

    void ReceivedHeaders(const fl::CallHeaders&) override {}
    void CallCompleted(const arrow::Status&) override {}

 private:
    std::string api_key_;
};

class ApiKeyMiddlewareFactory : public fl::ClientMiddlewareFactory {
 public:
    explicit ApiKeyMiddlewareFactory(std::string api_key)
        : api_key_(std::move(api_key)) {}

    void StartCall(const fl::CallInfo&,
                   std::unique_ptr<fl::ClientMiddleware>* middleware) override {
        *middleware = std::make_unique<ApiKeyMiddleware>(api_key_);
    }

 private:
    std::string api_key_;
};

// Clean an HTTP/2 header value.
// gRPC asserts-and-aborts on any byte outside the printable ASCII range
// (HTTP/2 §8.1.2.6 forbids CR/LF/NUL; `validate_metadata` is stricter).
// Strip leading/trailing whitespace, then reject outright if any
// non-printable byte remains in the middle — a silent strip could leave
// a truncated but still-valid-looking key and mask user error.
bool isValidHeaderValue(const std::string& s) {
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

std::string sanitizeHeaderValue(const std::string& s) {
    const auto is_trim = [](unsigned char c) {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    };
    size_t begin = 0;
    while (begin < s.size() && is_trim(static_cast<unsigned char>(s[begin]))) {
        ++begin;
    }
    size_t end = s.size();
    while (end > begin && is_trim(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    std::string trimmed = s.substr(begin, end - begin);
    // Any embedded control/non-ASCII byte would crash gRPC at call time;
    // drop the key entirely so the caller sees an unauthenticated attempt
    // rather than a hard abort.
    if (!isValidHeaderValue(trimmed)) return {};
    return trimmed;
}

// Same short tag the mosaico_client.cpp logs use, redeclared here so the
// pool's own log lines line up with the per-pull traces under `grep tid=`.
inline std::string poolTidStr() {
    const auto h = std::hash<std::thread::id>{}(std::this_thread::get_id());
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04u",
                  static_cast<unsigned>(h % 10000u));
    return std::string(buf);
}

}  // namespace

// ---------------------------------------------------------------------------
// ConnectionPool
// ---------------------------------------------------------------------------

ConnectionPool::ConnectionPool(const std::string& server_uri,
                               int timeout_seconds, size_t pool_size,
                               const std::string& tls_cert_path,
                               const std::string& api_key)
    : server_uri_(server_uri),
      timeout_(timeout_seconds),
      pool_size_(pool_size),
      tls_cert_path_(tls_cert_path),
      api_key_(sanitizeHeaderValue(api_key)),
      clients_(pool_size),
      slot_state_(pool_size, SlotState::Free) {}

ConnectionPool::~ConnectionPool() = default;

arrow::Result<std::unique_ptr<fl::FlightClient>>
ConnectionPool::createConnection() {
    ARROW_ASSIGN_OR_RAISE(auto location, fl::Location::Parse(server_uri_));
    fl::FlightClientOptions opts;

    if (!tls_cert_path_.empty()) {
        std::ifstream f(tls_cert_path_);
        if (!f) {
            return arrow::Status::IOError(
                "Cannot open TLS certificate file: " + tls_cert_path_);
        }
        std::stringstream ss;
        ss << f.rdbuf();
        opts.tls_root_certs = ss.str();
    }

    if (!api_key_.empty()) {
        opts.middleware.push_back(
            std::make_shared<ApiKeyMiddlewareFactory>(api_key_));
    }

    return fl::FlightClient::Connect(location, opts);
}

arrow::Result<ConnectionPool::Handle> ConnectionPool::checkout() {
    const auto t_begin = std::chrono::steady_clock::now();
    const std::string tid = poolTidStr();
    {
        std::unique_lock<std::mutex> lock(mutex_);

        // Find a free pooled slot that already has a connection.
        for (size_t i = 0; i < pool_size_; ++i) {
            if (slot_state_[i] == SlotState::Free && clients_[i]) {
                slot_state_[i] = SlotState::InUse;
                const auto wait_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t_begin).count();
                MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s checkout slot=%zu "
                                "(reuse, wait=%lld ms)\n",
                                tid.c_str(), i, (long long)wait_ms);
                return Handle(this, i, clients_[i].get());
            }
        }

        // Find a free slot that needs lazy creation.
        for (size_t i = 0; i < pool_size_; ++i) {
            if (slot_state_[i] == SlotState::Free && !clients_[i]) {
                slot_state_[i] = SlotState::Connecting;

                // Release lock while connecting.
                lock.unlock();
                const auto t_connect = std::chrono::steady_clock::now();
                auto result = createConnection();
                const auto connect_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t_connect).count();
                lock.lock();

                if (!result.ok()) {
                    slot_state_[i] = SlotState::Free;
                    MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s checkout slot=%zu "
                                    "FAILED to connect (%lld ms): %s\n",
                                    tid.c_str(), i, (long long)connect_ms,
                                    result.status().ToString().c_str());
                    return result.status();
                }
                clients_[i] = std::move(*result);
                slot_state_[i] = SlotState::InUse;
                const auto wait_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - t_begin).count();
                MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s checkout slot=%zu "
                                "(lazy connect=%lld ms, total wait=%lld ms)\n",
                                tid.c_str(), i, (long long)connect_ms,
                                (long long)wait_ms);
                return Handle(this, i, clients_[i].get());
            }
        }
    }

    // All pooled slots in use — create an overflow connection (no lock needed).
    // This is the "pool exhausted" signal worth flagging: it means callers
    // outnumber pool_size_ at this instant.
    const auto t_overflow = std::chrono::steady_clock::now();
    auto overflow_result = createConnection();
    const auto overflow_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t_overflow).count();
    if (!overflow_result.ok()) {
        MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s OVERFLOW connect FAILED "
                        "(%lld ms): %s\n",
                        tid.c_str(), (long long)overflow_ms,
                        overflow_result.status().ToString().c_str());
        return overflow_result.status();
    }
    MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s checkout OVERFLOW "
                    "(connect=%lld ms) — pool of %zu fully in use\n",
                    tid.c_str(), (long long)overflow_ms, pool_size_);
    return Handle(std::move(*overflow_result));
}

void ConnectionPool::returnConnection(size_t index) {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(index < pool_size_);
    slot_state_[index] = SlotState::Free;
    MOSAICO_SDK_LOG("[Mosaico SDK] pool: tid=%s release slot=%zu\n",
                    poolTidStr().c_str(), index);
}

// ---------------------------------------------------------------------------
// Handle
// ---------------------------------------------------------------------------

ConnectionPool::Handle::Handle(ConnectionPool* pool, size_t index,
                               fl::FlightClient* client)
    : pool_(pool), index_(index), client_(client), overflow_(false) {}

ConnectionPool::Handle::Handle(
    std::unique_ptr<fl::FlightClient> overflow_client)
    : overflow_client_(std::move(overflow_client)), overflow_(true) {
    client_ = overflow_client_.get();
}

ConnectionPool::Handle::Handle(Handle&& other) noexcept
    : pool_(other.pool_),
      index_(other.index_),
      client_(other.client_),
      overflow_client_(std::move(other.overflow_client_)),
      overflow_(other.overflow_) {
    other.reset();
}

ConnectionPool::Handle& ConnectionPool::Handle::operator=(
    Handle&& other) noexcept {
    if (this != &other) {
        // Return our current connection first (if any).
        if (valid() && !overflow_ && pool_) {
            pool_->returnConnection(index_);
        }
        // overflow_client_ unique_ptr will be destroyed by assignment.

        pool_ = other.pool_;
        index_ = other.index_;
        client_ = other.client_;
        overflow_client_ = std::move(other.overflow_client_);
        overflow_ = other.overflow_;
        other.reset();
    }
    return *this;
}

ConnectionPool::Handle::~Handle() {
    if (!valid()) return;
    if (!overflow_ && pool_) {
        pool_->returnConnection(index_);
    }
    // Overflow: overflow_client_ unique_ptr destroys the client.
}

fl::FlightClient* ConnectionPool::Handle::operator->() const {
    return client_;
}

fl::FlightClient& ConnectionPool::Handle::operator*() const {
    return *client_;
}

bool ConnectionPool::Handle::valid() const { return client_ != nullptr; }

void ConnectionPool::Handle::reset() noexcept {
    pool_ = nullptr;
    index_ = 0;
    client_ = nullptr;
    overflow_ = false;
    // Don't reset overflow_client_ — it was already moved.
}

} // namespace mosaico
