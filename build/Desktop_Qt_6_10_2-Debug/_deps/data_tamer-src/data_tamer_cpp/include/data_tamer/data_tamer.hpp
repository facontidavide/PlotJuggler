#pragma once

#include "data_tamer/channel.hpp"

namespace DataTamer
{

class ChannelsRegistry
{
public:
  ChannelsRegistry();

  // the Pimpl idiom does not allow a default destructor
  ~ChannelsRegistry();

  // global instance (similar to singleton)
  static ChannelsRegistry& Global();

  /// Once added here, this sink will be automatically connected to
  /// any new channel created with getChannel()
  void addDefaultSink(std::shared_ptr<DataSinkBase> sink);

  /// Create a new channel or get a previously create one.
  [[nodiscard]] std::shared_ptr<LogChannel> getChannel(std::string const& channel_name);

  /// remove all channels and stored sinks
  void clear();

private:
  struct Pimpl;
  std::unique_ptr<Pimpl> _p;
};

}  // namespace DataTamer
