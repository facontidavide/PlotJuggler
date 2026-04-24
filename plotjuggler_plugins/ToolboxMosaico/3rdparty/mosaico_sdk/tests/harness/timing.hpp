#pragma once

#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace mosaico::test {

// Client-side waterfall timer. Record named phases, emit a JSON report.
// Not a pass/fail primitive — it's a trend file. Store one per test that
// cares, diff across runs to catch regressions in connect/RPC latency.
class TimingWaterfall {
 public:
    explicit TimingWaterfall(std::string label) : label_(std::move(label)) {
        t0_ = std::chrono::steady_clock::now();
    }

    // Record a phase completion, relative to construction.
    void mark(std::string name) {
        auto now = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - t0_).count();
        marks_.emplace_back(std::move(name), us);
    }

    // Write JSON to the given path. Absolute path preferred.
    void emit(const std::string& path) const;

    const std::string& label() const { return label_; }
    const std::vector<std::pair<std::string, int64_t>>& marks() const { return marks_; }

 private:
    std::string label_;
    std::chrono::steady_clock::time_point t0_;
    std::vector<std::pair<std::string, int64_t>> marks_;  // (name, micros since t0)
};

}  // namespace mosaico::test
