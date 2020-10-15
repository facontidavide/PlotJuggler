#ifndef PAL_STATISTICS_MSG_H
#define PAL_STATISTICS_MSG_H

#include <string>
#include <vector>
#include <map>

#include <ros/types.h>
#include <ros/serialization.h>
#include <std_msgs/Header.h>
#include "ros1_parser.h"

struct PalStatisticsNames_
{
  //    Header header
  //    string[] names
  //    uint32 names_version #This is increased each time names change
  std_msgs::Header header;
  std::vector<std::string> names;
  uint32_t names_version;
};

struct PalStatisticsValues_
{
  //    Header header
  //    float64[] names
  //    uint32 names_version #This is increased each time names change
  std_msgs::Header header;
  std::vector<double> values;
  uint32_t names_version;
};

//-----------------------------------------------------

namespace ros
{
namespace serialization
{
template <>
struct Serializer<::PalStatisticsNames_>
{
  template <typename Stream, typename T>
  inline static void allInOne(Stream& stream, T m)
  {
    stream.next(m.header);
    stream.next(m.names);
    stream.next(m.names_version);
  }
  ROS_DECLARE_ALLINONE_SERIALIZER
};

template <>
struct Serializer<::PalStatisticsValues_>
{
  template <typename Stream, typename T>
  inline static void allInOne(Stream& stream, T m)
  {
    stream.next(m.header);
    stream.next(m.values);
    stream.next(m.names_version);
  }
  ROS_DECLARE_ALLINONE_SERIALIZER
};

}  // namespace serialization
}  // namespace ros

//-----------------------------------------------------
// key is topic name + names version
static std::unordered_map<std::string, std::vector<std::string>> _stored_pal_statistics_names;

std::string get_statistics_base_name(const std::string &topic_name)
{
  return topic_name.substr(0, topic_name.rfind('/'));
}

std::string get_stored_statistics_id(const std::string &base_name, int names_version)
{
  return base_name + std::to_string(names_version);
}

class PalStatisticsNamesParser : public MessageParserBase
{
public:
  PalStatisticsNamesParser(const std::string& topic_name, PlotDataMapRef& plot_data)
    : MessageParserBase(topic_name, plot_data)
  {
  }

  virtual bool parseMessage(SerializedMessage msg, double timestamp) override
  {
    PalStatisticsNames_ pal_names;
    ros::serialization::IStream is(const_cast<uint8_t*>(msg.data()), msg.size());
    ros::serialization::deserialize(is, pal_names);
    const std::string names_id = get_stored_statistics_id(get_statistics_base_name(_topic_name),
                                                          pal_names.names_version);
    _stored_pal_statistics_names.insert({names_id,
                                         std::move(pal_names.names) });
    return true;
  }
};

//-----------------------------------------------------
class PalStatisticsValuesParser : public MessageParserBase
{
public:
  PalStatisticsValuesParser(const std::string& topic_name, PlotDataMapRef& plot_data)
    : MessageParserBase(topic_name, plot_data), _base_name(get_statistics_base_name(topic_name))
  {
  }

  virtual bool parseMessage(SerializedMessage msg, double timestamp) override
  {
    PalStatisticsValues_ pal_msg;
    ros::serialization::IStream is(const_cast<uint8_t*>(msg.data()), msg.size());
    ros::serialization::deserialize(is, pal_msg);

    auto& values = _data[pal_msg.names_version];

    double header_stamp = pal_msg.header.stamp.toSec();
    timestamp = (_use_header_stamp && header_stamp > 0) ? header_stamp : timestamp;

    const std::string names_id = get_stored_statistics_id(get_statistics_base_name(_topic_name),
                                                          pal_msg.names_version);
    auto names_it = _stored_pal_statistics_names.find(names_id);
    if (names_it == _stored_pal_statistics_names.end())
    {
      return false;  // missing vocabulary
    }
    const auto& names = names_it->second;

    if (pal_msg.values.size() != names.size())
    {
      return false;  // weird... skip
    }

    for (size_t index = 0; index < pal_msg.values.size(); index++)
    {
      const auto& name = names[index];
      if (index >= values.size())
      {
        values.emplace_back(&getSeries(_plot_data, _base_name + "/" + name));
      }
      values[index]->pushBack({ timestamp, pal_msg.values[index] });
    }
    return true;
  }

private:
  std::string _base_name;
  std::unordered_map<uint32_t, std::vector<PlotData*>> _data;
};

#endif  // PAL_STATISTICS_MSG_H
