#ifndef ROS_PARSER_H
#define ROS_PARSER_H

#include "PlotJuggler/messageparser_base.h"
#include "PlotJuggler/special_messages.h"
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
  std::string _topic;

  void appendRollPitchYaw(double timestamp);

  void parseHeader(PJ::Msg::Header& header);

  void parseDiagnosticMsg(const PJ::MessageRef serialized_msg, double &timestamp);

  void parseJointStateMsg(const PJ::MessageRef serialized_msg, double &timestamp);

  void parseTF2Msg(const PJ::MessageRef serialized_msg, double &timestamp);

  bool _contains_quaternion = false;
  bool _is_diangostic_msg = false;
  bool _is_jointstate_msg = false;
  bool _is_tf2_msg = false;
};

#endif // ROS_PARSER_H
