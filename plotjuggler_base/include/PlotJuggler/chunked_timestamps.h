/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_CHUNKED_TIMESTAMPS_H
#define PJ_CHUNKED_TIMESTAMPS_H

#include <vector>
#include <deque>
#include <memory>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <cstring>
#include <unordered_map>

#include "contrib/unordered_dense.hpp"

namespace PJ
{

struct TimestampChunk
{
  static constexpr uint32_t CAPACITY = 1024;

  double min_x = std::numeric_limits<double>::max();
  double max_x = std::numeric_limits<double>::lowest();
  uint32_t count = 0;
  uint32_t start_offset = 0;

  std::vector<double> values;

  double valueAt(uint32_t local_index) const
  {
    return values[start_offset + local_index];
  }

  double& valueRef(uint32_t local_index)
  {
    return values[start_offset + local_index];
  }
};

class ChunkedTimestamps
{
  std::deque<std::shared_ptr<TimestampChunk>> _chunks;
  size_t _total_size = 0;

public:
  size_t size() const
  {
    return _total_size;
  }

  bool empty() const
  {
    return _total_size == 0;
  }

  void clear()
  {
    _chunks.clear();
    _total_size = 0;
  }

  double operator[](size_t index) const
  {
    auto [ci, li] = locate(index);
    return _chunks[ci]->valueAt(li);
  }

  double front() const
  {
    return _chunks.front()->valueAt(0);
  }

  double back() const
  {
    auto& last = *_chunks.back();
    return last.valueAt(last.count - 1);
  }

  void push_back(double x)
  {
    if (_chunks.empty() || _chunks.back()->count == TimestampChunk::CAPACITY)
    {
      _chunks.push_back(std::make_shared<TimestampChunk>());
    }

    auto& chunk = *_chunks.back();
    chunk.values.push_back(x);
    chunk.count++;

    if (x < chunk.min_x)
    {
      chunk.min_x = x;
    }
    if (x > chunk.max_x)
    {
      chunk.max_x = x;
    }

    _total_size++;
  }

  void pop_front()
  {
    auto& front_ptr = _chunks.front();

    // COW: if chunk is shared, copy before mutating
    if (front_ptr.use_count() > 1)
    {
      front_ptr = std::make_shared<TimestampChunk>(*front_ptr);
    }

    auto& chunk = *front_ptr;
    chunk.start_offset++;
    chunk.count--;
    if (chunk.count == 0)
    {
      _chunks.pop_front();
    }
    _total_size--;
  }

  void insert(size_t index, double x)
  {
    if (index == _total_size)
    {
      push_back(x);
      return;
    }

    // Flatten, insert, rebuild
    std::vector<double> flat;
    flat.reserve(_total_size + 1);
    for (size_t i = 0; i < _total_size; i++)
    {
      flat.push_back(operator[](i));
    }
    flat.insert(flat.begin() + index, x);

    clear();
    for (auto& val : flat)
    {
      push_back(val);
    }
  }

  double& mutableRef(size_t index)
  {
    auto [ci, li] = locate(index);
    auto& chunk_ptr = _chunks[ci];

    // COW: if chunk is shared, copy before mutating
    if (chunk_ptr.use_count() > 1)
    {
      chunk_ptr = std::make_shared<TimestampChunk>(*chunk_ptr);
    }
    return chunk_ptr->valueRef(li);
  }

  size_t lowerBound(double x) const
  {
    if (_chunks.empty())
    {
      return 0;
    }

    // Binary search over chunks: find first chunk where max_x >= x
    size_t lo = 0;
    size_t hi = _chunks.size();
    while (lo < hi)
    {
      size_t mid = lo + (hi - lo) / 2;
      if (_chunks[mid]->max_x < x)
      {
        lo = mid + 1;
      }
      else
      {
        hi = mid;
      }
    }

    if (lo >= _chunks.size())
    {
      return _total_size;
    }

    // Compute global offset of chunk lo
    size_t global_offset = 0;
    for (size_t i = 0; i < lo; i++)
    {
      global_offset += _chunks[i]->count;
    }

    // Binary search within the chunk
    auto& chunk = *_chunks[lo];
    const double* data = chunk.values.data() + chunk.start_offset;
    const double* found = std::lower_bound(data, data + chunk.count, x);
    size_t local_offset = static_cast<size_t>(found - data);

    return global_offset + local_offset;
  }

  const std::deque<std::shared_ptr<TimestampChunk>>& chunks() const
  {
    return _chunks;
  }

  // Dedup support: return the last sealed (full) chunk, if any
  std::shared_ptr<TimestampChunk> lastSealedChunk() const
  {
    if (_chunks.size() < 2)
    {
      return nullptr;
    }
    auto& candidate = _chunks[_chunks.size() - 2];
    if (candidate->count == TimestampChunk::CAPACITY)
    {
      return candidate;
    }
    return nullptr;
  }

  size_t lastSealedChunkIndex() const
  {
    return _chunks.size() - 2;
  }

  void replaceChunk(size_t idx, std::shared_ptr<TimestampChunk> shared)
  {
    _chunks[idx] = std::move(shared);
  }

  // Forward iterator for range-for
  class ConstIterator
  {
    const ChunkedTimestamps* _owner;
    size_t _index;

  public:
    ConstIterator(const ChunkedTimestamps* owner, size_t index) : _owner(owner), _index(index)
    {
    }

    double operator*() const
    {
      return (*_owner)[_index];
    }

    ConstIterator& operator++()
    {
      ++_index;
      return *this;
    }

    bool operator!=(const ConstIterator& other) const
    {
      return _index != other._index;
    }
  };

  ConstIterator begin() const
  {
    return ConstIterator(this, 0);
  }

  ConstIterator end() const
  {
    return ConstIterator(this, _total_size);
  }

private:
  std::pair<size_t, uint32_t> locate(size_t index) const
  {
    auto& first = *_chunks.front();
    uint32_t first_count = first.count;
    if (index < first_count)
    {
      return { 0, static_cast<uint32_t>(index) };
    }
    index -= first_count;
    size_t chunk_idx = index / TimestampChunk::CAPACITY + 1;
    uint32_t local_idx = index % TimestampChunk::CAPACITY;
    return { chunk_idx, local_idx };
  }
};

class TimestampRegistry
{
public:
  std::shared_ptr<TimestampChunk> deduplicate(std::shared_ptr<TimestampChunk> chunk)
  {
    uint64_t h = hashChunk(*chunk);
    auto it = _map.find(h);
    if (it != _map.end() && chunksEqual(*it->second, *chunk))
    {
      return it->second;
    }
    _map[h] = chunk;
    return chunk;
  }

  void clear()
  {
    _map.clear();
  }

private:
  static uint64_t hashChunk(const TimestampChunk& chunk)
  {
    return ankerl::unordered_dense::detail::wyhash::hash(chunk.values.data() + chunk.start_offset,
                                                         chunk.count * sizeof(double));
  }

  static bool chunksEqual(const TimestampChunk& a, const TimestampChunk& b)
  {
    if (a.count != b.count)
    {
      return false;
    }
    return std::memcmp(a.values.data() + a.start_offset, b.values.data() + b.start_offset,
                       a.count * sizeof(double)) == 0;
  }

  std::unordered_map<uint64_t, std::shared_ptr<TimestampChunk>> _map;
};

}  // namespace PJ

#endif
