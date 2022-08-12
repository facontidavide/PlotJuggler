#ifndef ROS_PARSER_H
#define ROS_PARSER_H

#include "PlotJuggler/messageparser_base.h"
#include "rosx_introspection/ros_parser.hpp"

class ParserROS : public PJ::MessageParser
{
public:
  ParserROS(const std::string& topic_name,
             const std::string& type_name,
            const std::string& schema,
            RosMsgParser::Deserializer *deserializer,
            PJ::PlotDataMapRef& data);

  bool parseMessage(const PJ::MessageRef serialized_msg, double& timestamp) override;

  void setLargeArraysPolicy(bool clamp, unsigned max_size) override;

protected:
  RosMsgParser::Parser _parser;
  std::shared_ptr<RosMsgParser::Deserializer> _deserializer;
  RosMsgParser::FlatMessage _flat_msg;
};

#endif // ROS_PARSER_H
