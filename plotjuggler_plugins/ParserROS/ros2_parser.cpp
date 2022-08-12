#include <QSettings>
#include <QMessageBox>
#include "ros2_parser.h"
#include "ros_parser.h"
#include "PlotJuggler/fmt/format.h"



MessageParserPtr ParserFactoryROS2::createParser(const std::string& topic_name,
                                                 const std::string& schema,
                                                 PlotDataMapRef& data)
{
  if(schema.empty())
  {
    throw std::runtime_error("ParserFactoryROS2 requires a schema (message definition)");
  }

  return std::make_shared<ParserROS>(topic_name,
                                     schema,
                                     new RosMsgParser::ROS2_Deserializer,
                                     data);
}
