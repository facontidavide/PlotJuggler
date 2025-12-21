#include "rosx_introspection/ros_utils/ros2_helpers.hpp"
#include "message_definition_cache.hpp"

#include <rclcpp/rclcpp.hpp>
#include <rcutils/error_handling.h>

namespace RosMsgParser
{

RmwInterface::RmwSerializedPtr
RmwInterface::get_initialized_serialized_message(size_t capacity)
{
  auto msg = new rmw_serialized_message_t;
  *msg = rmw_get_zero_initialized_serialized_message();
  auto ret = rmw_serialized_message_init(msg, capacity, &rcutils_allocator_);
  if (ret != RCUTILS_RET_OK)
  {
    throw std::runtime_error("Error allocating resources for serialized message: " +
                             std::string(rcutils_get_error_string().str));
  }

  auto serialized_message =
      std::shared_ptr<rmw_serialized_message_t>(msg, [](rmw_serialized_message_t* msg) {
        int error = rmw_serialized_message_fini(msg);
        delete msg;
        if (error != RCUTILS_RET_OK)
        {
          RCUTILS_LOG_ERROR_NAMED("rosbag2_test_common", "Leaking memory. Error: %s",
                                  rcutils_get_error_string().str);
        }
      });
  return serialized_message;
}

RmwInterface::RmwInterface()
{
  rcutils_allocator_ = rcutils_get_default_allocator();
}

std::string GetMessageDefinition(const std::string& datatype)
{
  RosMsgParser::MessageDefinitionCache cache;
  return cache.get_full_text(datatype);
}

}  // namespace RosMsgParser
