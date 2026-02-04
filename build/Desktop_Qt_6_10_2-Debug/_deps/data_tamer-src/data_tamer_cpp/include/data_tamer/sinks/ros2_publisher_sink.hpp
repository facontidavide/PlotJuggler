#pragma once

#include "data_tamer/data_sink.hpp"
#include "data_tamer/details/locked_reference.hpp"
#include "data_tamer_msgs/msg/schemas.hpp"
#include "data_tamer_msgs/msg/snapshot.hpp"
#include <unordered_map>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

namespace DataTamer
{

class ROS2PublisherSink : public DataSinkBase
{
public:
  ROS2PublisherSink(std::shared_ptr<rclcpp::Node> node, const std::string& topic_prefix);

  ROS2PublisherSink(std::shared_ptr<rclcpp_lifecycle::LifecycleNode> node,
                    const std::string& topic_prefix);

  template <typename NodeT>
  void create_publishers(NodeT& node, const std::string& topic_prefix)
  {
    rclcpp::QoS schemas_qos{ rclcpp::KeepAll() };
    schemas_qos.reliable();
    schemas_qos.transient_local();  // latch

    const rclcpp::QoS data_qos{ rclcpp::KeepAll() };

    schema_publisher_ = node->template create_publisher<data_tamer_msgs::msg::Schemas>(
        topic_prefix + "/schemas", schemas_qos);
    data_publisher_ = node->template create_publisher<data_tamer_msgs::msg::Snapshot>(
        topic_prefix + "/data", data_qos);
  }

  void addChannel(const std::string& name, const Schema& schema) override;

  bool storeSnapshot(const Snapshot& snapshot) override;

private:
  std::unordered_map<std::string, Schema> schemas_;
  Mutex schema_mutex_;

  rclcpp::Publisher<data_tamer_msgs::msg::Schemas>::SharedPtr schema_publisher_;
  rclcpp::Publisher<data_tamer_msgs::msg::Snapshot>::SharedPtr data_publisher_;

  bool schema_changed_ = true;
  data_tamer_msgs::msg::Snapshot data_msg_;
};

}  // namespace DataTamer
