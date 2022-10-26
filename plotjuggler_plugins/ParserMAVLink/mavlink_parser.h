#pragma once


#include <QDebug>

#include "PlotJuggler/messageparser_base.h"
#define MAVLINK_USE_MESSAGE_INFO
#include <mavlink_types.h>
#include <mavlink.h>

using namespace PJ;

class MAVLinkParser : public MessageParser
{
public:
  MAVLinkParser(const std::string& topic_name,
                 PlotDataMapRef& data)
    : MessageParser(topic_name, data)
  {
    memset(&_status,            0, sizeof(_status));
    memset(&_message,           0, sizeof(_message));
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override;

protected:
    mavlink_message_t _message;
    mavlink_status_t _status;
};



