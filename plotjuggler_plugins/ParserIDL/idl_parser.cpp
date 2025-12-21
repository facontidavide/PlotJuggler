#include "idl_parser.hpp"
#include "dds_parser/dds_parser.hpp"
#include <memory>

using namespace PJ;

class MsgParserImpl : public MessageParser
{
public:
  MsgParserImpl(const std::string& topic_name,  //
                const std::string& type_name,   //
                const std::string& schema,      //
                PJ::PlotDataMapRef& data)
    : MessageParser(topic_name, data), topic_name_(topic_name)
  {
    parser_ = std::make_unique<DDS::Parser>("", DDS::FullName(type_name), schema);
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override
  {
    DDS::Span<const uint8_t> msgSpan(serialized_msg.data(), serialized_msg.size());
    parser_->parse(msgSpan, flat_message_);

    for (const auto& [key, var] : flat_message_.numerical_values)
    {
      auto it = tmp_plot_data_.find(key);
      if (it == tmp_plot_data_.end())
      {
        it = tmp_plot_data_.emplace(key, PJ::PlotData(key, {})).first;
      }
      it->second.pushBack({ timestamp, DDS::castToDouble(var) });
    }
    for (const auto& [key, value] : flat_message_.string_values)
    {
      auto it = tmp_string_series_.find(key);
      if (it == tmp_string_series_.end())
      {
        it = tmp_string_series_.emplace(key, PJ::StringSeries(key, {})).first;
      }
      it->second.pushBack({ timestamp, value });
    }
    return true;
  }

  ~MsgParserImpl() override
  {
    // Move the temporary data to the main plot data map
    for (auto& [key, series] : tmp_plot_data_)
    {
      getSeries(topic_name_ + key).clonePoints(std::move(series));
    }
    for (auto& [key, series] : tmp_string_series_)
    {
      getStringSeries(topic_name_ + key).clonePoints(std::move(series));
    }
  }

private:
  std::string topic_name_;
  std::unique_ptr<DDS::Parser> parser_;
  DDS::FlatMessage flat_message_;

  std::unordered_map<std::string, PJ::PlotData> tmp_plot_data_;
  std::unordered_map<std::string, PJ::StringSeries> tmp_string_series_;
};

MessageParserPtr ParserFactoryIDL::createParser(const std::string& topic_name,
                                                const std::string& type_name,
                                                const std::string& schema, PJ::PlotDataMapRef& data)
{
  return std::make_shared<MsgParserImpl>(topic_name, type_name, schema, data);
}
