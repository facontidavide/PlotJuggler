# ROS X Introspection

Unified successor of the following libraries:

- [ros_type_introspection](https://github.com/facontidavide/ros_type_introspection)
- [ros_msg_parser](https://github.com/facontidavide/ros_msg_parser)
- [ros2_introspection](https://github.com/facontidavide/ros2_introspection)

The library compiles either using:
- ROS1 (catkin),
- ROS2 (colcon/ament) or
- without any ROS dependency (vanilla cmake).

To parse any ROS message at runtime, it requires:

- The name of the type (for instance "sensors_msgs/JointState")
- The definition of the type
  (for instance [this one](http://docs.ros.org/en/noetic/api/sensor_msgs/html/msg/JointState.html)),
- The raw memory buffer to be deserialized into individual key/values pairs.

The raw memory buffer is usually obtained by:

- [rosbag::MessageInstance](https://docs.ros.org/en/noetic/api/rosbag_storage/html/c++/classrosbag_1_1MessageInstance.html) and
  [Topic::ShapeShifter](http://docs.ros.org/en/noetic/api/topic_tools/html/shape__shifter_8h.html)
  in **ROS1**.
- [GenericSubscriber](https://github.com/ros2/rclcpp/blob/rolling/rclcpp/src/rclcpp/generic_subscription.cpp)
  or `rosbag2_storage::SerializedBagMessage` on **ROS2**.
- [MCAP](https://github.com/foxglove/mcap).


