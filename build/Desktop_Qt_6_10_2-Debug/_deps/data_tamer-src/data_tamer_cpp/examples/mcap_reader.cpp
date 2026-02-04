#include "data_tamer_parser/data_tamer_parser.hpp"
#include <mcap/reader.hpp>

// Try reading the generated [test_sample.mcap]
// using examples/mcap_writer_sample.cpp
int main(int argc, char** argv)
{
  if(argc != 2)
  {
    std::cout << "add the MCAP file as argument" << std::endl;
    return 1;
  }
  std::string filepath = argv[1];

  // open the file
  mcap::McapReader reader;
  {
    auto const res = reader.open(filepath);
    if(!res.ok())
    {
      throw std::runtime_error("Can't open MCAP file");
    }
  }

  // start reading all the schemas and parsing them
  std::unordered_map<mcap::SchemaId, size_t> schema_id_to_hash;
  std::unordered_map<size_t, DataTamerParser::Schema> hash_to_schema;
  // must call this, before accessing the schemas
  auto summary = reader.readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
  for(const auto& [schema_id, mcap_schema] : reader.schemas())
  {
    std::string schema_text(reinterpret_cast<const char*>(mcap_schema->data.data()),
                            mcap_schema->data.size());

    auto dt_schema = DataTamerParser::BuilSchemaFromText(schema_text);
    schema_id_to_hash[mcap_schema->id] = dt_schema.hash;
    hash_to_schema[dt_schema.hash] = dt_schema;
  }

  // this application will do nothing with the actual data. We will simple count the
  // number of messages per time series
  using MessageCount = std::map<std::string, size_t>;
  std::map<std::string, MessageCount> message_counts_per_channel;

  auto IncrementCounter = [&](const std::string& series_name,
                              MessageCount& message_counts) {
    auto it = message_counts.find(series_name);
    if(it == message_counts.end())
    {
      message_counts[series_name] = 1;
    }
    else
    {
      it->second++;
    }
  };

  // parse all messages
  for(const auto& msg : reader.readMessages())
  {
    // start updating the fields of SnapshotView
    DataTamerParser::SnapshotView snapshot;
    snapshot.timestamp = msg.message.logTime;
    snapshot.schema_hash = schema_id_to_hash.at(msg.schema->id);
    const auto& dt_schema = hash_to_schema.at(snapshot.schema_hash);

    // msg_buffer contains both active_mask and payload, serialized
    // one after the other.
    DataTamerParser::BufferSpan msg_buffer = {
      reinterpret_cast<const uint8_t*>(msg.message.data), msg.message.dataSize
    };

    const uint32_t mask_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
    snapshot.active_mask.data = msg_buffer.data;
    snapshot.active_mask.size = mask_size;
    msg_buffer.trimFront(mask_size);

    const uint32_t payload_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
    snapshot.payload.data = msg_buffer.data;
    snapshot.payload.size = payload_size;

    // prepare the callback to be invoked by ParseSnapshot.
    // Wrap IncrementCounter to add the channel_name
    const std::string& channel_name = msg.channel->topic;
    auto& message_counts = message_counts_per_channel[channel_name];
    auto callback_number = [&](const std::string& series_name,
                               const DataTamerParser::VarNumber&) {
      IncrementCounter(series_name, message_counts);
    };

    DataTamerParser::ParseSnapshot(dt_schema, snapshot, callback_number);
  }

  // display the counted data samples
  for(const auto& [channel_name, msg_counts] : message_counts_per_channel)
  {
    std::cout << channel_name << ":" << std::endl;
    for(const auto& [name, count] : msg_counts)
    {
      std::cout << "   " << name << ":" << count << std::endl;
    }
  }
  return 0;
}
