/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_CHUNKED_VALUES_H
#define PJ_CHUNKED_VALUES_H

#include <vector>
#include <deque>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <numeric>

namespace PJ
{

template <typename Value>
struct ValueChunk
{
  static constexpr uint32_t CAPACITY = 1024;

  double min_y = std::numeric_limits<double>::max();
  double max_y = std::numeric_limits<double>::lowest();
  uint32_t count = 0;
  uint32_t start_offset = 0;

  std::vector<Value> values;

  bool isCompressed() const
  {
    return values.empty() && count > 0;
  }

  Value constantY() const
  {
    return static_cast<Value>(min_y);
  }

  Value valueAt(uint32_t local_index) const
  {
    if (isCompressed())
    {
      return constantY();
    }
    return values[start_offset + local_index];
  }

  Value& valueRef(uint32_t local_index)
  {
    if (isCompressed())
    {
      decompress();
    }
    return values[start_offset + local_index];
  }

  void decompress()
  {
    Value y = constantY();
    values.resize(start_offset + count, y);
  }
};

template <typename Value>
class ChunkedValues
{
  std::deque<ValueChunk<Value>> _chunks;
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

  Value valueAt(size_t index) const
  {
    auto [ci, li] = locate(index);
    return _chunks[ci].valueAt(li);
  }

  Value& valueRef(size_t index)
  {
    auto [ci, li] = locate(index);
    return _chunks[ci].valueRef(li);
  }

  void push_back(const Value& v)
  {
    if (_chunks.empty() || _chunks.back().count == ValueChunk<Value>::CAPACITY)
    {
      if (!_chunks.empty())
      {
        tryCompress(_chunks.size() - 1);
      }
      _chunks.emplace_back();
    }

    auto& chunk = _chunks.back();
    chunk.values.push_back(v);
    chunk.count++;

    if constexpr (std::is_arithmetic_v<Value>)
    {
      double dv = static_cast<double>(v);
      if (dv < chunk.min_y)
      {
        chunk.min_y = dv;
      }
      if (dv > chunk.max_y)
      {
        chunk.max_y = dv;
      }
    }

    _total_size++;
  }

  void pop_front()
  {
    auto& chunk = _chunks.front();
    chunk.start_offset++;
    chunk.count--;
    if (chunk.count == 0)
    {
      _chunks.pop_front();
    }
    _total_size--;
  }

  void insert(size_t index, const Value& v)
  {
    if (index == _total_size)
    {
      push_back(v);
      return;
    }

    // For simplicity: flatten, insert, rebuild
    std::vector<Value> flat;
    flat.reserve(_total_size + 1);
    for (size_t i = 0; i < _total_size; i++)
    {
      flat.push_back(valueAt(i));
    }
    flat.insert(flat.begin() + index, v);

    clear();
    for (auto& val : flat)
    {
      push_back(val);
    }
  }

  const std::deque<ValueChunk<Value>>& chunks() const
  {
    return _chunks;
  }

private:
  std::pair<size_t, uint32_t> locate(size_t index) const
  {
    auto& first = _chunks.front();
    uint32_t first_count = first.count;
    if (index < first_count)
    {
      return { 0, static_cast<uint32_t>(index) };
    }
    index -= first_count;
    size_t chunk_idx = index / ValueChunk<Value>::CAPACITY + 1;
    uint32_t local_idx = index % ValueChunk<Value>::CAPACITY;
    return { chunk_idx, local_idx };
  }

  void tryCompress(size_t chunk_idx)
  {
    if constexpr (!std::is_arithmetic_v<Value>)
    {
      return;
    }
    else
    {
      auto& chunk = _chunks[chunk_idx];
      if (chunk.min_y == chunk.max_y)
      {
        chunk.values.clear();
        chunk.values.shrink_to_fit();
      }
    }
  }
};

}  // namespace PJ

#endif
