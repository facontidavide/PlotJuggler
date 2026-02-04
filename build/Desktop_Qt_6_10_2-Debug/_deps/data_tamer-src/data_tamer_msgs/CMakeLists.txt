cmake_minimum_required(VERSION 3.8)
project(data_tamer_msgs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Schema.msg"
  "msg/Schemas.msg"
  "msg/Snapshot.msg"
)

ament_export_dependencies(rosidl_default_runtime)
ament_package()
