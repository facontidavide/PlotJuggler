#include "data_tamer/channel.hpp"
#include "data_tamer/data_sink.hpp"
#include "data_tamer/contrib/SerializeMe.hpp"

#include <unordered_map>
#include <unordered_set>

namespace DataTamer
{

struct LogChannel::Pimpl
{
  struct ValueHolder
  {
    std::string name;
    bool enabled = true;
    bool registered = true;
    ValuePtr holder;
  };

  std::string channel_name;

  mutable Mutex mutex;

  std::vector<ValueHolder> series;
  std::unordered_map<std::string, size_t> registered_values;

  bool mask_dirty = true;

  Snapshot snapshot;
  Schema schema;
  bool logging_started = false;

  std::unordered_set<std::shared_ptr<DataSinkBase>> sinks;
};

RegistrationID LogChannel::registerValueImpl(const std::string& name,
                                             ValuePtr&& value_ptr,
                                             CustomSerializer::Ptr type_info)
{
  if(name.find(' ') != std::string::npos)
  {
    throw std::runtime_error("name can not contain spaces");
  }

  std::lock_guard const lock(_p->mutex);
  _p->mask_dirty = true;

  // check if this name exists already
  auto it = _p->registered_values.find(name);
  if(it == _p->registered_values.end())
  {
    if(_p->logging_started)
    {
      throw std::runtime_error("Can't register a new value once recording started, "
                               "i.e. after takeShapshot was called the first time");
    }
    // appending a new ValueHolder to series
    const auto type = value_ptr.type();
    const std::string type_name = type_info ? type_info->typeName() : ToStr(type);
    TypeField field{ name, type, type_name, value_ptr.isVector(),
                     value_ptr.vectorSize() };

    Pimpl::ValueHolder instance;
    instance.name = name;
    instance.holder = std::move(value_ptr);
    _p->series.emplace_back(std::move(instance));

    const size_t index = _p->series.size() - 1;

    _p->registered_values.insert({ name, index });

    // update schema and its hash (append only)
    _p->schema.hash = AddFieldToHash(field, _p->schema.hash);
    _p->schema.fields.emplace_back(std::move(field));

    // if this was a special serializer with its own schema, save it instead in custom_schemas
    if(type_info)
    {
      auto custom_schema = type_info->typeSchema();
      if(custom_schema && _p->schema.custom_types.count(type_info->typeName()) == 0)
      {
        _p->schema.custom_schemas.insert({ type_info->typeName(), *custom_schema });
      }
    }

    return { index, 1 };
  }

  // trying to registered again an unregistered holder or to
  // overwite its holder
  const size_t index = it->second;
  auto& instance = _p->series[index];

  if(instance.registered)
  {
    throw std::runtime_error("This name was already registered. Unregister it first");
  }

  // check if the new holder is compatible
  if(instance.holder != value_ptr)
  {
    throw std::runtime_error("Can't change the type of a previously "
                             "registered value");
  }

  // if it was marked as NOT registered, we should registered it again
  if(!instance.registered)
  {
    instance.registered = true;
  }
  instance.enabled = true;
  instance.holder = std::move(value_ptr);
  return { index, 1 };
}

LogChannel::LogChannel(std::string name) : _p(new Pimpl)
{
  _p->schema.hash = std::hash<std::string>()(name);
  _p->schema.channel_name = name;
  _p->channel_name = std::move(name);
}

std::shared_ptr<LogChannel> LogChannel::create(std::string name)
{
  return std::shared_ptr<LogChannel>(new LogChannel(std::move(name)));
}

const std::string& LogChannel::channelName() const
{
  return _p->channel_name;
}

LogChannel::~LogChannel() {}

void LogChannel::setEnabled(const RegistrationID& id, bool enable)
{
  std::lock_guard const lock(_p->mutex);
  for(size_t i = 0; i < id.fields_count; i++)
  {
    auto& instance = _p->series[id.first_index + i];
    if(instance.enabled != enable)
    {
      instance.enabled = enable;
      _p->mask_dirty = true;
    }
  }
}

void LogChannel::unregister(const RegistrationID& id)
{
  std::lock_guard const lock(_p->mutex);
  for(size_t i = 0; i < id.fields_count; i++)
  {
    auto& instance = _p->series[id.first_index + i];
    instance.registered = false;
    instance.enabled = false;
  }
}

void LogChannel::addDataSink(std::shared_ptr<DataSinkBase> sink)
{
  _p->sinks.insert(sink);
}

Schema LogChannel::getSchema() const
{
  std::lock_guard const lock(_p->mutex);
  return _p->schema;
}

Mutex& LogChannel::writeMutex()
{
  return _p->mutex;
}

void LogChannel::addCustomType(const std::string& custom_type_name,
                               const FieldsVector& fields)
{
  _p->schema.custom_types[custom_type_name] = fields;
}

bool LogChannel::takeSnapshot(std::chrono::nanoseconds timestamp)
{
  {
    std::lock_guard const lock(_p->mutex);

    if(_p->sinks.empty())
    {
      return false;
    }
    // update the _p->snapshot.active_mask if necessary
    if(_p->mask_dirty)
    {
      _p->mask_dirty = false;
      auto& mask = _p->snapshot.active_mask;
      mask.clear();
      const auto vect_size = (_p->series.size() + 7) / 8;  // ceiling size
      mask.resize(vect_size, 0xFF);
      for(size_t i = 0; i < _p->series.size(); i++)
      {
        auto const& instance = _p->series[i];
        if(!instance.enabled)
        {
          SetBit(mask, i, false);
        }
      }
    }

    size_t payload_size = 0;
    for(size_t i = 0; i < _p->series.size(); i++)
    {
      auto const& instance = _p->series[i];
      payload_size += instance.holder.getSerializedSize();
    }
    _p->snapshot.payload.resize(payload_size);

    // call sink->addChannel (usually done once)
    if(!_p->logging_started)
    {
      _p->logging_started = true;
      _p->snapshot.schema_hash = _p->schema.hash;
      for(auto const& sink : _p->sinks)
      {
        sink->addChannel(_p->channel_name, _p->schema);
      }
    }

    _p->snapshot.timestamp = timestamp;
    _p->snapshot.channel_name = channelName();

    // serialize data into _p->snapshot.payload
    SerializeMe::SpanBytes payload_buffer(_p->snapshot.payload);

    for(auto const& entry : _p->series)
    {
      if(entry.enabled)
      {
        entry.holder.serialize(payload_buffer);
      }
    }
    _p->snapshot.payload.resize(_p->snapshot.payload.size() - payload_buffer.size());
  }

  bool all_pushed = true;
  for(auto& sink : _p->sinks)
  {
    all_pushed &= sink->pushSnapshot(_p->snapshot);
  }
  return all_pushed;
}

}  // namespace DataTamer
