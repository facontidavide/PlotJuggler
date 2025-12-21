cmake_minimum_required(VERSION 3.16)

project(rosx_introspection LANGUAGES CXX VERSION 1.0.2)

# create compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable PIC only for the rosx_introspection target

###############################################
## Declare a C++ library
###############################################
add_library(rosx_introspection STATIC
    src/ros_type.cpp
    src/ros_field.cpp
    src/stringtree_leaf.cpp
    src/ros_message.cpp
    src/ros_parser.cpp
    src/deserializer.cpp
    src/serializer.cpp
    )

set_target_properties(rosx_introspection PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(rosx_introspection PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_compile_features(rosx_introspection PUBLIC cxx_std_17)
