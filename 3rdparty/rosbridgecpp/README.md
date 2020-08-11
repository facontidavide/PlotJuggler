## Usage
See examples in rosbridge_ws_client.cpp

## To include in your project
Add these lines to your CMakeLists.txt
```cmake
include_directories(<rosbridgecpp source_dir>)
add_subdirectory(<rosbridgecpp source_dir> rosbridgecpp)
target_link_libraries(<target> rosbridgecpp)
```
Example:
```cmake
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../rosbridgecpp)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../rosbridgecpp rosbridgecpp)
target_link_libraries(${PROJECT_NAME} rosbridgecpp)
```
