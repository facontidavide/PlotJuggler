#pragma once

#include "data_tamer/data_sink.hpp"

#include <mutex>
#include <unordered_map>
#include <shared_mutex>

namespace DataTamer
{

using Mutex = std::shared_mutex;

/**
 * @brief The DummySink does nothing, only counting the number of snapshots received.
 * Used mostly for testing and debugging.
 */
class DummySink : public DataSinkBase
{
public:
  std::unordered_map<uint64_t, Schema> schemas;
  std::unordered_map<uint64_t, std::string> schema_names;
  std::unordered_map<uint64_t, long> snapshots_count;
  Snapshot latest_snapshot;
  Mutex schema_mutex_;

  ~DummySink() override { stopThread(); }

  void addChannel(std::string const& name, Schema const& schema) override
  {
    std::scoped_lock lk(schema_mutex_);
    schemas[schema.hash] = schema;
    schema_names[schema.hash] = name;
    snapshots_count[schema.hash] = 0;
  }

  bool storeSnapshot(const Snapshot& snapshot) override
  {
    std::scoped_lock lk(schema_mutex_);
    latest_snapshot = snapshot;

    auto it = snapshots_count.find(snapshot.schema_hash);
    if(it != snapshots_count.end())
    {
      it->second++;
    }
    return true;
  }
};

}  // namespace DataTamer
