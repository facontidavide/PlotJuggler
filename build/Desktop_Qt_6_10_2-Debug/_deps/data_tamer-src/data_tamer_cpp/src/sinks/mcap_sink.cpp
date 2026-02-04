#include "data_tamer/sinks/mcap_sink.hpp"
#include "data_tamer/contrib/SerializeMe.hpp"

#include <chrono>
#include <sstream>
#include <mutex>
#include <string>

#ifndef USING_ROS2
#define MCAP_IMPLEMENTATION
#endif

#include <mcap/writer.hpp>
#include <mcap/reader.hpp>

#if defined __has_include && __has_include("boost/container/small_vector.hpp")
#include <boost/container/small_vector.hpp>

namespace SerializeMe
{
template <size_t N>
void SerializeIntoBuffer(SpanBytes& buffer,
                         boost::container::small_vector<uint8_t, N> const& value)
{
  SerializeMe::SerializeIntoBuffer(buffer, uint32_t(value.size()));
  std::memcpy(buffer.data(), value.data(), value.size());
  buffer.trimFront(value.size());
}
}  // end namespace SerializeMe

#endif

namespace DataTamer
{

static constexpr char const* kDataTamer = "data_tamer";

MCAPSink::MCAPSink(const std::string& filepath, bool do_compression)
  : filepath_(filepath), compression_(do_compression), original_filepath_(filepath)
{
  openFile(filepath_);
}

void DataTamer::MCAPSink::openFile(std::string const& filepath)
{
  std::scoped_lock lk(mutex_);
  writer_ = std::make_unique<mcap::McapWriter>();
  mcap::McapWriterOptions options(kDataTamer);
  options.compression = compression_ ? mcap::Compression::Zstd : mcap::Compression::None;
  auto status = writer_->open(filepath, options);
  if(!status.ok())
  {
    throw std::runtime_error("Failed to open MCAP file for writing");
  }
  start_time_ = std::chrono::system_clock::now();
  // clean up, in case this was opened a second time
  hash_to_channel_id_.clear();
}

MCAPSink::~MCAPSink()
{
  stopThread();
  std::scoped_lock lk(mutex_);
}

void MCAPSink::addChannel(std::string const& channel_name, Schema const& schema)
{
  std::scoped_lock lk(mutex_);
  schemas_[channel_name] = schema;
  auto it = hash_to_channel_id_.find(schema.hash);
  if(it != hash_to_channel_id_.end())
  {
    return;
  }

  std::stringstream ss;
  ss << schema;
  std::string schema_str = ss.str();

  auto const schema_name = channel_name + "::" + std::to_string(schema.hash);

  // Register a Schema
  mcap::Schema mcap_schema(schema_name, kDataTamer, schema_str);
  writer_->addSchema(mcap_schema);

  // Register a Channel
  mcap::Channel publisher(channel_name, kDataTamer, mcap_schema.id);
  writer_->addChannel(publisher);
  hash_to_channel_id_[schema.hash] = publisher.id;
}

bool MCAPSink::storeSnapshot(const Snapshot& snapshot)
{
  std::scoped_lock lk(mutex_);
  if(forced_stop_recording_)
  {
    return false;
  }
  // the payload must contain both the ActiveMask and the other data
  thread_local std::vector<uint8_t> merged_payload;
  const auto size_mask = snapshot.active_mask.size();
  const auto size_data = snapshot.payload.size();

  merged_payload.resize(size_mask + size_data + sizeof(uint32_t) * 2);
  SerializeMe::SpanBytes buffer(merged_payload);
  SerializeMe::SerializeIntoBuffer(buffer, snapshot.active_mask);
  SerializeMe::SerializeIntoBuffer(buffer, snapshot.payload);

  // Write our message
  mcap::Message msg;
  msg.channelId = hash_to_channel_id_.at(snapshot.schema_hash);
  msg.sequence = 1;  // Optional
  // Timestamp requires nanosecond
  msg.logTime = mcap::Timestamp(snapshot.timestamp.count());
  msg.publishTime = msg.logTime;
  msg.data = reinterpret_cast<std::byte const*>(merged_payload.data());  // NOLINT
  msg.dataSize = merged_payload.size();
  auto status = writer_->write(msg);

  // If reset_time_ is exceeded, we want to overwrite the current file.
  // Better than filling the disk, if you forgot to stop the application.
  auto const now = std::chrono::system_clock::now();
  if(reset_time_ != std::chrono::seconds(0) && now - start_time_ > reset_time_)
  {
    if(create_file_on_reset_)
    {
      // change the current filepath to the original with "_[# resets]"" appended
      filepath_ = original_filepath_ + "_" + std::to_string(file_reset_counter_);
      ++file_reset_counter_;
    }
    restartRecordingImpl(filepath_, compression_, false);
  }
  return true;
}

void MCAPSink::setMaxTimeBeforeReset(std::chrono::seconds reset_time)
{
  reset_time_ = reset_time;
}

void MCAPSink::setCreateNewFileOnReset(bool create_file_on_reset)
{
  create_file_on_reset_ = create_file_on_reset;
}

void MCAPSink::stopRecording()
{
  std::scoped_lock lk(mutex_);
  forced_stop_recording_ = true;
  writer_->close();
  writer_.reset();
}

void MCAPSink::restartRecording(const std::string& filepath, bool do_compression)
{
  restartRecordingImpl(filepath, do_compression, true);
}

void MCAPSink::restartRecordingImpl(const std::string& filepath, bool do_compression,
                                    bool new_file)
{
  std::scoped_lock lk(mutex_);
  if(new_file)
  {
    // if this was called by a user, we need to change the filepath that we will increment when reset
    file_reset_counter_ = 1;
    original_filepath_ = filepath;
  }
  filepath_ = filepath;
  compression_ = do_compression;
  openFile(filepath_);

  // rebuild the channels
  for(auto const& [name, schema] : schemas_)
  {
    addChannel(name, schema);
  }
}

}  // namespace DataTamer
