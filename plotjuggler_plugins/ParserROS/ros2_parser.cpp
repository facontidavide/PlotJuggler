#include <QSettings>
#include <QMessageBox>
#include "ros2_parser.h"
#include "ros_parser.h"
#include "PlotJuggler/fmt/format.h"



MessageParserPtr ParserFactoryROS2::createParser(const std::string& topic_name,
                                                 const std::string &type_name,
                                                 const std::string& schema,
                                                 PlotDataMapRef& data)
{
  if(schema.empty())
  {
    throw std::runtime_error("ParserFactoryROS2 requires a schema (message definition)");
  }

  std::string msg_type = QString::fromStdString(type_name).replace("/msg/", "/").toStdString();

  return std::make_shared<ParserROS>(topic_name,
                                     msg_type,
                                     schema,
                                     new RosMsgParser::ROS2_Deserializer,
                                     data);
}
