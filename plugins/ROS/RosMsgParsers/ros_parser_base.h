#ifndef ROS_MESSAGEPARSER_H
#define ROS_MESSAGEPARSER_H

#include "PlotJuggler/messageparser_base.h"
#include <ros/types.h>
#include <ros/serialization.h>
#include <ros/builtin_message_traits.h>
#include <ros/message_operations.h>
#include <std_msgs/Header.h>
#include <fmt/format.h>


class ApplyPrefix
{
  std::string _add_prefix = {};
  std::vector<std::string> _rm_prefixes= {};
  bool _null_transform = true;
public:
  ApplyPrefix(){};
  ApplyPrefix(const std::string & add_prefix, const std::vector<std::string> & rm_prefix)
  : _add_prefix(add_prefix)
  , _rm_prefixes(rm_prefix)
  , _null_transform(_add_prefix.empty() && _rm_prefixes.size() == 0)
  {
  }

  const std::string * apply(const std::string& topic, std::string & out) const
  {
    if (_null_transform)
      return &topic;

    unsigned int skip = 0;
    for (const std::string & rm_prefix : _rm_prefixes)
    {
      if (topic.compare(0,rm_prefix.size(),rm_prefix) == 0)
      {
        skip = rm_prefix.size();
      }
    }

    out.clear();
    out.reserve(_add_prefix.size() + topic.size() - skip);
    out.assign(_add_prefix);
    out.append(topic, skip, std::string::npos);
    return &out;
  }

  std::string apply(const std::string& topic) const
  {
    for (const std::string & rm_prefix : _rm_prefixes)
    {
      if (topic.compare(0,rm_prefix.size(),rm_prefix) == 0)
      {
        return _add_prefix + topic.substr(rm_prefix.length());
      }
    }

    return _add_prefix + topic;
  }

};





class RosParserBase : public MessageParser
{
public:

    RosParserBase(): _use_header_stamp(false){
    }

    virtual void setUseHeaderStamp( bool use )
    {
        _use_header_stamp = use;
    }

    virtual void setTopicTransform(const std::string& prefix, const std::vector<std::string>& rm_prefix )
    {
      _prefixer = ApplyPrefix(prefix,rm_prefix);
    }

    virtual std::string getPrefixedTopic(const std::string& topic)
    {
      return _prefixer.apply(topic);
    }


protected:
    bool _use_header_stamp;
    ApplyPrefix _prefixer;

};

template <typename MainType, typename SubType, class ChildParser>
class RosMessageStampedParser: public RosParserBase
{
public:
    RosMessageStampedParser( const char* child_prefix):
        _child_prefix(child_prefix)
    {
        _data.emplace_back( "/header/seq" );
        _data.emplace_back( "/header/stamp" );
    }


    const std::unordered_set<std::string>& getCompatibleKeys() const override
    {
        static std::unordered_set<std::string> compatible_key =
        { ros::message_traits::MD5Sum<MainType>::value() };
        return compatible_key;
    }

    virtual void pushMessageRef(const std::string& key,
                                const MessageRef& buffer,
                                double timestamp) override
    {
        std_msgs::Header header;
        ros::serialization::IStream is( const_cast<uint8_t*>(buffer.data()), buffer.size() );
        ros::serialization::deserialize(is, header);

        size_t header_size = sizeof( ros::Time ) + 8 + header.frame_id.size();

        _data[0].pushBack( {timestamp, header.seq} );
        _data[1].pushBack( {timestamp, header.stamp.toSec()} );

        MessageRef sub_buffer( buffer.data(), header_size);

        _child_parser.pushMessageRef( key, sub_buffer, timestamp );
    }

    void setTopicTransform(const std::string& prefix, const std::vector<std::string> &rm_prefix ) override
    {
        _child_parser.setTopicTransform(prefix,rm_prefix);
    }

    void extractData(PlotDataMapRef& plot_map, const std::string& prefix) override
    {
        for (auto& it: _data)
        {
            MessageParser::appendData(plot_map, this->_prefixer.apply(it.name()), it);
        }
        _child_parser.extractData(plot_map, prefix + _child_prefix);
    }
private:
    std::vector<PlotData> _data;
    ChildParser _child_parser;
    std::string _child_prefix;

};



#endif // ROS_MESSAGEPARSER_H
