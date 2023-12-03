#include <sstream>
#include <iostream>

#include "datatamer_parser.h"
#include "data_tamer_parser/data_tamer_parser.hpp"
#include "PlotJuggler/fmt/format.h"

using namespace PJ;


class MsgParserImpl: public MessageParser
{
  public:
  MsgParserImpl(const std::string &topic_name,
                const std::string &type_name,
                const std::string &schema,
                PJ::PlotDataMapRef &data):
    MessageParser(topic_name, data),
            topic_name_(topic_name)
  {
    schema_ = DataTamerParser::BuilSchemaFromText(schema);
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override
  {
    auto callback = [this, timestamp](const std::string& series_name,
                                      const DataTamerParser::VarNumber& var)
    {
      auto name = fmt::format("{}/{}", topic_name_, series_name);
      auto& plot_data = _plot_data.getOrCreateNumeric(name);

      double value =  std::visit([](auto&& v) { return static_cast<double>(v); },
                                 var);

      plot_data.pushBack({timestamp, value});
    };

    DataTamerParser::SnapshotView snapshot;
    snapshot.schema_hash = schema_.hash;

    DataTamerParser::BufferSpan msg_buffer =
        { serialized_msg.data(), serialized_msg.size() };

    const uint32_t mask_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
    snapshot.active_mask.data = msg_buffer.data;
    snapshot.active_mask.size = mask_size;
    msg_buffer.trimFront(mask_size);

    const uint32_t payload_size = DataTamerParser::Deserialize<uint32_t>(msg_buffer);
    snapshot.payload.data = msg_buffer.data;
    snapshot.payload.size = payload_size;

    DataTamerParser::ParseSnapshot(schema_, snapshot, callback);
    return true;
  }

  private:

  DataTamerParser::Schema schema_;

  struct TimeSeries {
    std::string name;
    DataTamerParser::BasicType type;
    PlotData* plot_data = nullptr;
  };

  std::string topic_name_;
  std::vector<TimeSeries> timeseries_;
};


MessageParserPtr ParserDataTamer::createParser(const std::string &topic_name,
                                               const std::string &type_name,
                                               const std::string &schema,
                                               PJ::PlotDataMapRef &data)
{
  return std::make_shared<MsgParserImpl>(topic_name, type_name, schema, data);
}
