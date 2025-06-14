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
    parser_ = std::make_unique<DDS::Parser>(topic_name, DDS::FullName(type_name), schema);
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override
  {
    DDS::Span<const uint8_t> msgSpan(serialized_msg.data(), serialized_msg.size());
    parser_->parse(msgSpan, flat_message_);

    for (const auto& [key, var] : flat_message_.numerical_values)
    {
      getSeries(key).pushBack({ timestamp, DDS::castToDouble(var) });
    }
    for (const auto& [key, value] : flat_message_.string_values)
    {
      getStringSeries(key).pushBack({ timestamp, value });
    }

    return true;
  }

private:
  std::string topic_name_;
  std::unique_ptr<DDS::Parser> parser_;
  DDS::FlatMessage flat_message_;
};

MessageParserPtr ParserFactoryIDL::createParser(const std::string& topic_name,
                                                const std::string& type_name,
                                                const std::string& schema,
                                                PJ::PlotDataMapRef& data)
{
  return std::make_shared<MsgParserImpl>(topic_name, type_name, schema, data);
}
