#include <QSettings>
#include <QMessageBox>
#include "protobuf_parser.h"
#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"


using namespace google::protobuf;
using namespace google::protobuf::io;
using namespace google::protobuf::compiler;

bool ProtobufParser::parseMessage(const MessageRef serialized_msg,
                                  double &timestamp)
{
  const google::protobuf::Message* prototype_msg =
    _msg_factory.GetPrototype(_msg_descriptor);

  google::protobuf::Message* mutable_msg = prototype_msg->New();
  if (!mutable_msg->ParseFromArray(serialized_msg.data(),
                                   serialized_msg.size()))
  {
    return false;
  }

  std::function<void(const google::protobuf::Message&, const std::string&)> ParseImpl;

  ParseImpl = [&](const google::protobuf::Message& msg, const std::string& prefix)
  {
    const Reflection* reflection = msg.GetReflection();
    const Descriptor* descriptor = msg.GetDescriptor();
    //    std::vector<const FieldDescriptor*> reflection_fields;
    //    reflection->ListFields(msg, &reflection_fields);

    for (int index=0; index < descriptor->field_count(); index++)
    {
      auto field = descriptor->field(index);

      std::string key = prefix.empty() ?
                          field->name():
                          fmt::format("{}/{}", prefix, field->name() );
      std::string suffix;

      if (!field)
      {
        continue;
      }

      int count = 1;
      bool repeated = false;
      if (field->is_repeated())
      {
        count = reflection->FieldSize(msg, field);
        repeated = true;
      }

      for(int index = 0; index < count ; index++)
      {
        if(repeated)
        {
          suffix = fmt::format("[{}]", index);
        }

        bool is_double = true;
        double value = 0;
        switch(field->cpp_type())
        {
          case FieldDescriptor::CPPTYPE_DOUBLE:{
            value = !repeated ? reflection->GetDouble(msg, field) :
                                reflection->GetRepeatedDouble(msg, field, index);
          }break;
          case FieldDescriptor::CPPTYPE_FLOAT:{
            auto tmp = !repeated ? reflection->GetFloat(msg, field) :
                                   reflection->GetRepeatedFloat(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT32:{
            auto tmp = !repeated ? reflection->GetUInt32(msg, field) :
                                   reflection->GetRepeatedUInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_UINT64:{
            auto tmp = !repeated ? reflection->GetUInt64(msg, field) :
                                   reflection->GetRepeatedUInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_BOOL:{
            auto tmp = !repeated ? reflection->GetBool(msg, field) :
                                   reflection->GetRepeatedBool(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT32:{
            auto tmp = !repeated ? reflection->GetInt32(msg, field) :
                                   reflection->GetRepeatedInt32(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_INT64:{
            auto tmp = !repeated ? reflection->GetInt64(msg, field) :
                                   reflection->GetRepeatedInt64(msg, field, index);
            value = static_cast<double>(tmp);
          }break;
          case FieldDescriptor::CPPTYPE_ENUM:{
            auto tmp = !repeated ? reflection->GetEnum(msg, field) :
                                   reflection->GetRepeatedEnum(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp->name()});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_STRING:{
            auto tmp = !repeated ? reflection->GetString(msg, field) :
                                   reflection->GetRepeatedString(msg, field, index);

            auto& series = this->getStringSeries(key + suffix);
            series.pushBack({timestamp, tmp});
            is_double = false;
          }break;
          case FieldDescriptor::CPPTYPE_MESSAGE:
          {
// Fix macro issue in Windows
#pragma push_macro("GetMessage")
#undef GetMessage
            const auto& new_msg = reflection->GetMessage(msg, field, nullptr);
#pragma pop_macro("GetMessage")

            ParseImpl(new_msg, key + suffix);
            is_double = false;
          }break;
        }

        if( is_double )
        {
          auto& series = this->getSeries(key + suffix);
          series.pushBack({timestamp, value});
        }
      }
    }
  };

  // start recursion
  ParseImpl(*mutable_msg, _topic_name);

  return true;
}

