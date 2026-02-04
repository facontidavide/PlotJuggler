#pragma once

#include "data_tamer/types.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace DataTamer
{

using ActiveMask = std::vector<uint8_t>;
using PayloadVector = std::vector<uint8_t>;

bool GetBit(const ActiveMask& mask, size_t index);
void SetBit(ActiveMask& mask, size_t index, bool val);

struct Snapshot
{
  std::string_view channel_name;

  /// Unique identifier of the schema
  std::size_t schema_hash;

  /// snapshot timestamp
  std::chrono::nanoseconds timestamp;

  /// Vector that tell us if a field of the schema is
  /// active or not. It is basically an optimized vector
  /// of bools, where each byte contains 8 boolean flags.
  ActiveMask active_mask;

  /// serialized dat containing all the values, ordered as in the schema
  PayloadVector payload;
};

/**
 * @brief The DataSnapshot contains all the information passed by
 * LogChannel::takeSnapshot to a DataSink.
 *
 * Note: The vector contains:
 *
 * - The serialized SnapshotHeader in the first bytes
 * - The payload in the following bytes
 */
using DataSnapshot = std::vector<uint8_t>;

/**
 * @brief The DataSinkBase is the base class to use to create
 * your own DataSink
 */
class DataSinkBase
{
public:
  DataSinkBase();

  DataSinkBase(const DataSinkBase& other) = delete;
  DataSinkBase& operator=(const DataSinkBase& other) = delete;

  DataSinkBase(DataSinkBase&& other) = delete;
  DataSinkBase& operator=(DataSinkBase&& other) = delete;

  virtual ~DataSinkBase();

  /**
   * @brief addChannel will register a schema into the sink.
   * That schema will be recognized by its hash.
   *
   * @param name    name of the channel
   * @param schema  a schema, suaully obtained from LogChannel::getSchema()
   */
  virtual void addChannel(const std::string& name, const Schema& schema) = 0;

  /**
   * @brief pushSnapshot will push the data into a concurrent queue,
   * that a different thread will consume, using storeSnapshot()
   *
   * @param snapshot see type Snapshot for details
   *
   * @return false if the queue is full and snapshot was not pushed
   */
  virtual bool pushSnapshot(const Snapshot& snapshot);

protected:
  /**
   * @brief storeSnapshot contains the code to execute when popping a snapshot from
   * the queue.
   *
   * @param snapshot data to be pushed into the sink.
   * @return true if processed successfully
   */
  virtual bool storeSnapshot(const Snapshot& snapshot) = 0;

  void stopThread();

private:
  struct Pimpl;
  std::unique_ptr<Pimpl> _p;
};

//--------------------------------------------------

inline bool GetBit(const ActiveMask& mask, size_t index)
{
  const uint8_t& byte = mask[index >> 3];
  return 0 != (byte & uint8_t(1 << (index % 8)));
}

inline void SetBit(ActiveMask& mask, size_t index, bool value)
{
  if(!value)
  {
    mask[index >> 3] &= uint8_t(~(1 << (index % 8)));
  }
  else
  {
    mask[index >> 3] |= uint8_t(1 << (index % 8));
  }
}

}  // namespace DataTamer
