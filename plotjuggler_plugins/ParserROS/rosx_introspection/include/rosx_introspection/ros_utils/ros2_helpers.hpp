#pragma once

#include <rclcpp/rclcpp.hpp>
#include <rosbag2_cpp/types/introspection_message.hpp>
#include <rosbag2_cpp/typesupport_helpers.hpp>
#include <rosidl_typesupport_cpp/identifier.hpp>
#include <rosidl_typesupport_introspection_cpp/identifier.hpp>

#include "rosx_introspection/ros_field.hpp"

namespace RosMsgParser {

class RmwInterface {
 public:
  using RmwSerializedPtr = std::shared_ptr<rmw_serialized_message_t>;

  RmwInterface();
  ~RmwInterface() = default;

  template <typename T>
  RmwSerializedPtr serialize_message(const T& message, const rosidl_message_type_support_t* type_support) {
    auto serialized_message = get_initialized_serialized_message(0);
    auto error = rmw_serialize(&message, type_support, serialized_message.get());
    if (error != RCL_RET_OK) {
      throw std::runtime_error("Failed to serialize");
    }
    return serialized_message;
  }

  template <typename T>
  std::shared_ptr<T> deserialize_message(
      RmwSerializedPtr serialized_msg, const rosidl_message_type_support_t* type_support) {
    auto message = std::make_shared<T>();
    auto error = rmw_deserialize(
        serialized_msg.get(),
        // get_message_typesupport(message),
        type_support, message.get());
    if (error != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED("rosbag2_test_common", "Leaking memory. Error: %s", rcutils_get_error_string().str);
    }
    return message;
  }

 private:
  RmwSerializedPtr get_initialized_serialized_message(size_t capacity);
  rcutils_allocator_t rcutils_allocator_;
};

std::string GetMessageDefinition(const std::string& datatype);

template <typename T>
inline std::vector<uint8_t> BuildMessageBuffer(const T& msg, const std::string& topic_type) {
  const auto& ts_identifier = rosidl_typesupport_cpp::typesupport_identifier;
  auto ts_library = rosbag2_cpp::get_typesupport_library(topic_type, ts_identifier);
  auto typesupport = rosbag2_cpp::get_typesupport_handle(topic_type, ts_identifier, ts_library);

  RmwInterface rmw;
  auto serialized_msg = rmw.serialize_message(msg, typesupport);
  std::vector<uint8_t> buffer(serialized_msg->buffer_length, 0);
  memcpy(buffer.data(), serialized_msg->buffer, serialized_msg->buffer_length);
  return buffer;
}

template <typename T>
inline T BufferToMessage(const void* bufferPtr, size_t bufferSize) {
  // get the type name of the ROS message with traits
  const std::string topic_type = rosidl_generator_traits::name<T>();
  const auto& ts_identifier = rosidl_typesupport_cpp::typesupport_identifier;
  auto ts_library = rosbag2_cpp::get_typesupport_library(topic_type, ts_identifier);
  auto type_support = rosbag2_cpp::get_typesupport_handle(topic_type, ts_identifier, ts_library);

  rmw_serialized_message_t serialized_msg;
  serialized_msg.buffer_capacity = bufferSize;
  serialized_msg.buffer_length = bufferSize;
  serialized_msg.buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(bufferPtr));

  T message;
  auto res = rmw_deserialize(&serialized_msg, type_support, &message);
  return message;
}

template <typename T>
inline T BufferToMessage(const std::vector<uint8_t>& buffer) {
  return BufferToMessage<T>(buffer.data(), buffer.size());
}

}  // namespace RosMsgParser
