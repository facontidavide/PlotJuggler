#include "ros1_parser.h"
#include "jointstates_msg.h"
#include "imu_msg.h"
#include "diagnostic_msg.h"
#include "odometry_msg.h"
#include "pal_statistics_msg.h"
#include "tf_msg.h"
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
  _parser.setMaxArrayPolicy(static_cast<RosIntrospection::Parser::MaxArrayPolicy>(discard_policy));
  _max_size = max_size;
}

bool IntrospectionParser::parseMessage(SerializedMessage serialized_msg, double timestamp)
{
  _parser.deserializeIntoFlatContainer(_topic_name, serialized_msg, &_flat_msg, _max_size);

  if (_use_header_stamp)
  {
    for (const auto& it : _flat_msg.value)
    {
      if (it.second.getTypeID() != RosIntrospection::TIME)
      {
        continue;
      }
      const RosIntrospection::StringTreeNode* leaf1 = it.first.node_ptr;
      const RosIntrospection::StringTreeNode* leaf2 = leaf1->parent();
      if (leaf2 && leaf2->value() == "header" && leaf1->value() == "stamp")
      {
        double header_stamp = it.second.convert<double>();

        if (header_stamp > 0)
        {
          timestamp = header_stamp;
        }
        break;
      }
    }
  }

  //  if(_use_header_stamp && _intropection_parser.topicInfo().has_header_stamp)
  //  {
  //    double sec  = _flat_msg.values[0].second;
  //    double nsec = _flat_msg.values[1].second;
  //    timestamp = sec + (nsec*1e-9);
  //  }

  _parser.applyNameTransform(_topic_name, _flat_msg, &_renamed);

  for (const auto& it : _renamed)
  {
    const auto& key = it.first;
    double value = it.second.convert<double>();

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

void CompositeParser::registerMessageType(const std::string& topic_name,
                                          const std::string& topic_type,
                                          const std::string& definition)
{
  std::shared_ptr<MessageParserBase> parser;
  if (_parsers.count(topic_name) > 0)
  {
    return;
  }

  const std::string& type = topic_type;

  if (type == "sensor_msgs/JointState")
  {
    parser.reset(new JointStateMsgParser(topic_name, _plot_data));
  }
  else if (type == "diagnostic_msgs/DiagnosticArray")
  {
    parser.reset(new DiagnosticMsgParser(topic_name, _plot_data));
  }
  else if (type == "tf/tfMessage")
  {
    parser.reset(new TfMsgParser(topic_name, _plot_data));
  }
  else if (type == "tf2_msgs/TFMessage")
  {
    parser.reset(new Tf2MsgParser(topic_name, _plot_data));
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
  else if (type == "pal_statistics_msgs/StatisticsNames")
  {
    parser.reset(new PalStatisticsNamesParser(topic_name, _plot_data));
  }
  else if (type == "pal_statistics_msgs/StatisticsValues")
  {
    parser.reset(new PalStatisticsValuesParser(topic_name, _plot_data));
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
    parser.reset(new IntrospectionParser(topic_name, type, definition, _plot_data));
  }

  parser->setMaxArrayPolicy(_discard_policy, _max_array_size);
  parser->setUseHeaderStamp(_use_header_stamp);
  _parsers.insert({ topic_name, parser });
}

bool CompositeParser::parseMessage(const std::string& topic_name, SerializedMessage serialized_msg, double timestamp)
{
  auto it = _parsers.find(topic_name);
  if (it == _parsers.end())
  {
    return false;
  }
  it->second->parseMessage(serialized_msg, timestamp);
  return false;
}
