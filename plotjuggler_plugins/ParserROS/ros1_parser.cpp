#include <QSettings>
#include <QMessageBox>
#include "ros1_parser.h"
#include "ros_parser.h"
#include "PlotJuggler/fmt/format.h"

MessageParserPtr ParserFactoryROS1::createParser(const std::string& topic_name,
                                                 const std::string &type_name,
                                                 const std::string& schema,
                                                 PlotDataMapRef& data)
{
  if(schema.empty())
  {
    throw std::runtime_error("ParserFactoryROS1 requires a schema (message definition)");
  }

  return std::make_shared<ParserROS>(topic_name,
                                     type_name,
                                     schema,
                                     new RosMsgParser::ROS_Deserializer(),
                                     data);
}


