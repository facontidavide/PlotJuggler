#include "ros2_parser.h"
#include "jointstates_msg.h"
#include "imu_msg.h"
#include "odometry_msg.h"
#include "plotjuggler_msgs.h"


void MessageParserBase::setUseHeaderStamp(bool use)
{
  _use_header_stamp = use;
}

PlotData& MessageParserBase::getSeries(PlotDataMapRef& plot_data, const std::string key)
{
  auto plot_pair = plot_data.numeric.find(key);
  if (plot_pair == plot_data.numeric.end())
  {
    plot_pair = plot_data.addNumeric(key);
  }
  return plot_pair->second;
}

//-------------------------------------
void IntrospectionParser::setMaxArrayPolicy(LargeArrayPolicy discard_policy, size_t max_size)
{
  _intropection_parser.setMaxArrayPolicy(static_cast<Ros2Introspection::MaxArrayPolicy>(discard_policy), max_size);
}

bool IntrospectionParser::parseMessage(const rcutils_uint8_array_t* serialized_msg, double timestamp)
{
  _intropection_parser.deserializeIntoFlatMessage(serialized_msg, &_flat_msg);

  if (_use_header_stamp && _intropection_parser.topicInfo().has_header_stamp)
  {
    double sec = _flat_msg.values[0].second;
    double nsec = _flat_msg.values[1].second;
    timestamp = sec + (nsec * 1e-9);
  }

  ConvertFlatMessageToRenamedValues(_flat_msg, _renamed);

  for (const auto& it : _renamed)
  {
    const auto& key = it.first;
    double value = it.second;

    auto& series = getSeries(_plot_data, key);

    if (!std::isnan(value) && !std::isinf(value))
    {
      series.pushBack({ timestamp, value });
    }
  }
  return true;
}

//-----------------------------------------

CompositeParser::CompositeParser(PlotDataMapRef& plot_data)
  : _discard_policy(LargeArrayPolicy::DISCARD_LARGE_ARRAYS)
  , _max_array_size(999)
  , _use_header_stamp(false)
  , _plot_data(plot_data)
{
}

void CompositeParser::setUseHeaderStamp(bool use)
{
  _use_header_stamp = use;
  for (auto it : _parsers)
  {
    it.second->setUseHeaderStamp(use);
  }
}

void CompositeParser::setMaxArrayPolicy(LargeArrayPolicy policy, size_t max_size)
{
  _discard_policy = policy;
  _max_array_size = max_size;
  for (auto it : _parsers)
  {
    it.second->setMaxArrayPolicy(policy, max_size);
  }
}

void CompositeParser::registerMessageType(const std::string& topic_name, const std::string& topic_type)
{
  std::shared_ptr<MessageParserBase> parser;
  if (_parsers.count(topic_name) > 0)
  {
    return;
  }

  std::string type = topic_type;

  // replace verbose name
  size_t str_index = type.find("/msg/", 0);
  if (str_index != std::string::npos)
  {
    type.erase(str_index, 4);
  }

  if (type == "sensor_msgs/JointState")
  {
    parser.reset(new JointStateMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/Quaternion")
  {
    parser.reset(new QuaternionMsgParser(topic_name, _plot_data));
  }
  else if (type == "sensor_msgs/Imu")
  {
    parser.reset(new ImuMsgParser(topic_name, _plot_data));
  }
  else if (type == "nav_msgs/Odometry")
  {
    parser.reset(new OdometryMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/Pose")
  {
    parser.reset(new PoseMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/PoseStamped")
  {
    parser.reset(new PoseStampedMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/PoseWithCovariance")
  {
    parser.reset(new PoseCovarianceMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/PoseWithCovarianceStamped")
  {
    parser.reset(new PoseCovarianceStampedMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/Twist")
  {
    parser.reset(new TwistMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/TwistStamped")
  {
    parser.reset(new TwistStampedMsgParser(topic_name, _plot_data));
  }
  else if (type == "geometry_msgs/TwistWithCovariance")
  {
    parser.reset(new TwistCovarianceMsgParser(topic_name, _plot_data));
  }
  else if (type == "plotjuggler_msgs/Dictionary")
  {
    parser.reset(new PlotJugglerDictionaryParser(topic_name, _plot_data));
  }
  else if (type == "plotjuggler_msgs/DataPoints")
  {
    parser.reset(new PlotJugglerDataPointsParser(topic_name, _plot_data));
  }
  else
  {
    parser.reset(new IntrospectionParser(topic_name, type, _plot_data));
  }

  parser->setMaxArrayPolicy(_discard_policy, _max_array_size);
  parser->setUseHeaderStamp(_use_header_stamp);
  _parsers.insert({ topic_name, parser });
}

bool CompositeParser::parseMessage(const std::string& topic_name, const SerializedMessage* serialized_msg,
                                   double timestamp)
{
  auto it = _parsers.find(topic_name);
  if (it == _parsers.end())
  {
    return false;
  }
  it->second->parseMessage(serialized_msg, timestamp);
  return false;
}

const rosidl_message_type_support_t* CompositeParser::typeSupport(const std::string& topic_name) const
{
  auto it = _parsers.find(topic_name);
  if (it == _parsers.end())
  {
    return nullptr;
  }
  return it->second->typeSupport();
}
