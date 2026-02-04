#include "data_tamer/data_tamer.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace DataTamer
{

struct ChannelsRegistry::Pimpl
{
  std::unordered_map<std::string, std::shared_ptr<LogChannel>> channels;
  std::unordered_set<std::shared_ptr<DataSinkBase>> default_sinks;
  Mutex mutex;
};

ChannelsRegistry::ChannelsRegistry() : _p(new Pimpl) {}

ChannelsRegistry::~ChannelsRegistry() {}

ChannelsRegistry& ChannelsRegistry::Global()
{
  static ChannelsRegistry obj;
  return obj;
}

void ChannelsRegistry::addDefaultSink(std::shared_ptr<DataSinkBase> sink)
{
  std::scoped_lock lk(_p->mutex);
  _p->default_sinks.insert(sink);
}

std::shared_ptr<LogChannel> ChannelsRegistry::getChannel(std::string const& channel_name)
{
  std::scoped_lock lk(_p->mutex);
  auto it = _p->channels.find(channel_name);
  if(it == _p->channels.end())
  {
    auto new_channel = LogChannel::create(channel_name);
    for(auto const& sink : _p->default_sinks)
    {
      new_channel->addDataSink(sink);
    }
    _p->channels.insert({ channel_name, new_channel });
    return new_channel;
  }
  return it->second;
}

void ChannelsRegistry::clear()
{
  std::scoped_lock lk(_p->mutex);
  _p->channels.clear();
  _p->default_sinks.clear();
}

}  // namespace DataTamer
