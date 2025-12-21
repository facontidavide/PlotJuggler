#pragma once

#include "types.hpp"
#include <algorithm>
#include <variant>

namespace mcap::internal {

// Helper for writing compile-time exhaustive variant visitors.
template <class>
inline constexpr bool always_false_v = false;

/**
 * @brief A job to read a specific message at offset `offset` from the decompressed chunk
 * stored in `chunkReaderIndex`. A timestamp is provided to order this job relative to other jobs.
 */
struct ReadMessageJob {
  Timestamp timestamp;
  RecordOffset offset;
  size_t chunkReaderIndex;
};

/**
 * @brief A job to decompress the chunk starting at `chunkStartOffset`. The message indices
 * starting directly after the chunk record and ending at `messageIndexEndOffset` will be used to
 * find specific messages within the chunk.
 */
struct DecompressChunkJob {
  Timestamp messageStartTime;
  Timestamp messageEndTime;
  ByteOffset chunkStartOffset;
  ByteOffset messageIndexEndOffset;
};

/**
 * @brief A union of jobs that an indexed MCAP reader executes.
 */
using ReadJob = std::variant<ReadMessageJob, DecompressChunkJob>;

/**
 * @brief A priority queue of jobs for an indexed MCAP reader to execute.
 */
struct ReadJobQueue {
private:
  bool reverse_ = false;
  std::vector<ReadJob> heap_;

  /**
   * @brief return the timestamp key that should be used to compare jobs.
   */
  static Timestamp TimeComparisonKey(const ReadJob& job, bool reverse) {
    Timestamp result = 0;
    std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ReadMessageJob>) {
          result = arg.timestamp;
        } else if constexpr (std::is_same_v<T, DecompressChunkJob>) {
          if (reverse) {
            result = arg.messageEndTime;
          } else {
            result = arg.messageStartTime;
          }
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
      },
      job);
    return result;
  }
  static RecordOffset PositionComparisonKey(const ReadJob& job, bool reverse) {
    RecordOffset result;
    std::visit(
      [&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, ReadMessageJob>) {
          result = arg.offset;
        } else if constexpr (std::is_same_v<T, DecompressChunkJob>) {
          if (reverse) {
            result.offset = arg.messageIndexEndOffset;
          } else {
            result.offset = arg.chunkStartOffset;
          }
        } else {
          static_assert(always_false_v<T>, "non-exhaustive visitor!");
        }
      },
      job);
    return result;
  }

  static bool CompareForward(const ReadJob& a, const ReadJob& b) {
    auto aTimestamp = TimeComparisonKey(a, false);
    auto bTimestamp = TimeComparisonKey(b, false);
    if (aTimestamp == bTimestamp) {
      return PositionComparisonKey(a, false) > PositionComparisonKey(b, false);
    }
    return aTimestamp > bTimestamp;
  }

  static bool CompareReverse(const ReadJob& a, const ReadJob& b) {
    auto aTimestamp = TimeComparisonKey(a, true);
    auto bTimestamp = TimeComparisonKey(b, true);
    if (aTimestamp == bTimestamp) {
      return PositionComparisonKey(a, true) < PositionComparisonKey(b, true);
    }
    return aTimestamp < bTimestamp;
  }

public:
  explicit ReadJobQueue(bool reverse)
      : reverse_(reverse) {}
  void push(DecompressChunkJob&& decompressChunkJob) {
    heap_.emplace_back(std::move(decompressChunkJob));
    if (!reverse_) {
      std::push_heap(heap_.begin(), heap_.end(), CompareForward);
    } else {
      std::push_heap(heap_.begin(), heap_.end(), CompareReverse);
    }
  }

  void push(ReadMessageJob&& readMessageJob) {
    heap_.emplace_back(std::move(readMessageJob));
    if (!reverse_) {
      std::push_heap(heap_.begin(), heap_.end(), CompareForward);
    } else {
      std::push_heap(heap_.begin(), heap_.end(), CompareReverse);
    }
  }

  ReadJob pop() {
    if (!reverse_) {
      std::pop_heap(heap_.begin(), heap_.end(), CompareForward);
    } else {
      std::pop_heap(heap_.begin(), heap_.end(), CompareReverse);
    }
    auto popped = heap_.back();
    heap_.pop_back();
    return popped;
  }

  size_t len() const {
    return heap_.size();
  }
};

}  // namespace mcap::internal
