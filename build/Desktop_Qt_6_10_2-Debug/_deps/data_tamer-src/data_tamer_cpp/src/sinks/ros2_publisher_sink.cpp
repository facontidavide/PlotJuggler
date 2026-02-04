#include "data_tamer/sinks/ros2_publisher_sink.hpp"

namespace DataTamer
{
ROS2PublisherSink::ROS2PublisherSink(std::shared_ptr<rclcpp::Node> node,
                                     const std::string& topic_prefix)
{
  create_publishers(node, topic_prefix);
}

ROS2PublisherSink::ROS2PublisherSink(
    std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node,
    const std::string& topic_prefix)
{
  create_publishers(node, topic_prefix);
}

void ROS2PublisherSink::addChannel(const std::string& channel_name, const Schema& schema)
{
  std::scoped_lock lk(schema_mutex_);
  schemas_[channel_name] = schema;
  schema_changed_ = true;
}

bool ROS2PublisherSink::storeSnapshot(const Snapshot& snapshot)
{
  // send the schemas, if you haven't yet.
  if(schema_changed_)
  {
    std::scoped_lock lk(schema_mutex_);
    schema_changed_ = false;
    data_tamer_msgs::msg::Schemas msg;
    msg.schemas.reserve(schemas_.size());

    for(const auto& [channel_name, schema] : schemas_)
    {
      data_tamer_msgs::msg::Schema schema_msg;
      schema_msg.hash = schema.hash;
      schema_msg.channel_name = channel_name;
      std::ostringstream ss;
      ss << schema;
      schema_msg.schema_text = ss.str();

      msg.schemas.push_back(std::move(schema_msg));
    }
    schema_publisher_->publish(msg);
  }
  //----------------------------------------
  data_msg_.timestamp_nsec = uint64_t(snapshot.timestamp.count());
  data_msg_.schema_hash = snapshot.schema_hash;
  data_msg_.active_mask = snapshot.active_mask;
  data_msg_.payload = snapshot.payload;
  data_publisher_->publish(data_msg_);

  return true;
}

}  // namespace DataTamer
