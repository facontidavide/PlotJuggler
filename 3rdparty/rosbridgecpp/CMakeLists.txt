cmake_minimum_required(VERSION 3.0)
project(rosbridgecpp)

option(PRINT_DEBUG "Print debug messages" ON)

if(PRINT_DEBUG)
  add_definitions(-DDEBUG)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF) # Set OFF to use -std=c++11, otherwise it uses -std=gnu++11 by default

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Simple-WebSocket-Server") #PROJECT_SOURCE_DIR
add_subdirectory(Simple-WebSocket-Server)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include")

add_library(rosbridgecpp INTERFACE)
target_include_directories(rosbridgecpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rosbridgecpp INTERFACE simple-websocket-server)

# If Simple-WebSocket-Server is not a sub-project:
#if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  add_executable(rosbridge_ws_client rosbridge_ws_client.cpp)
  target_link_libraries(rosbridge_ws_client simple-websocket-server)
#endif()
