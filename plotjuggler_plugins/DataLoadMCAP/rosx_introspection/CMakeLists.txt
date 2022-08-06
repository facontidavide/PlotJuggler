cmake_minimum_required(VERSION 3.10)

project(rosx_introspection  LANGUAGES C CXX VERSION 2.0.0)

# Build flags
#---- Enable C++17 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Override Fast-CDR option: compile as static lib

find_package(fastcdr QUIET)

if(NOT fastcdr_FOUND )
    SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Create shared libraries by default")
    add_subdirectory(3rdparty/Fast-CDR)
endif()

###############################################
## Identify the compilation type
###############################################
# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR catkin_FOUND OR CATKIN_BUILD_BINARY_PACKAGE)

    set(COMPILING_WITH_CATKIN ON)
    message(STATUS "Compiling with CATKIN.")

elseif( DEFINED ENV{AMENT_PREFIX_PATH})

    set(COMPILING_WITH_AMENT ON)
    message(STATUS "Compiling with AMENT.")
    set(ROS_UTILS_SRC
        src/ros_utils/message_definition_cache.cpp
        src/ros_utils/ros2_helpers.cpp)
else()

    set(NO_ROS_DEPENDENCIES ON)
    message(STATUS "Compiling without ROS.")

endif()

###############################################
## Declare a C++ library
###############################################
add_library(rosx_introspection
    ${SRC_FILES}
    src/ros_type.cpp
    src/ros_field.cpp
    src/stringtree_leaf.cpp
    src/ros_message.cpp
    src/ros_parser.cpp
    src/deserializer.cpp
    ${ROS_UTILS_SRC}
    )
target_link_libraries(rosx_introspection
    fastcdr
    ${fastcdr_LIBRARIES})

target_include_directories(rosx_introspection PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Fast-CDR/include>
    $<INSTALL_INTERFACE:include>
    )

###############################################
## Install the package
###############################################
if(COMPILING_WITH_CATKIN)

    set(PACKAGE_DEPEND
        roscpp
        roscpp_serialization
        sensor_msgs)

    find_package(catkin REQUIRED COMPONENTS
        ${PACKAGE_DEPEND})

    catkin_package(
        INCLUDE_DIRS  include
        LIBRARIES  rosx_introspection
        CATKIN_DEPENDS ${PACKAGE_DEPEND}
        )

elseif( COMPILING_WITH_AMENT )

    find_package(ament_cmake REQUIRED)
    find_package(rclcpp REQUIRED)

    ament_target_dependencies(rosx_introspection rclcpp )

    ament_export_include_directories(include)
    ament_export_libraries(my_library)

    install(
        DIRECTORY include/
        DESTINATION include
        )

    install(
        TARGETS rosx_introspection
        EXPORT rosx_introspectionTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )

endif()

###############################################
## Compile the tests
###############################################

option(INTROSPECTION_TESTS "Compile the tests" ON)

if(INTROSPECTION_TESTS AND COMPILING_WITH_CATKIN )

    include_directories(${catkin_INCLUDE_DIRS})

    add_executable(test_parser
        test/test_parser.cpp
        test/test_ros1.cpp )

    target_link_libraries(test_parser
        rosx_introspection
        ${catkin_LIBRARIES})
endif()

if(INTROSPECTION_TESTS AND COMPILING_WITH_AMENT)

    find_package(rosbag2_cpp REQUIRED)
    find_package(rosidl_typesupport_cpp REQUIRED)
    find_package(sensor_msgs REQUIRED)
    find_package(geometry_msgs REQUIRED)

    set( PACKAGE_DEPEND
        rclcpp
        rosbag2_cpp
        rosidl_typesupport_cpp
        sensor_msgs )

    add_executable(test_parser
        test/test_parser.cpp
        test/test_ros2.cpp
        )

    ament_target_dependencies(test_parser ${PACKAGE_DEPEND})
    target_link_libraries(test_parser rosx_introspection )

endif( )

if(INTROSPECTION_TESTS AND NO_ROS_DEPENDENCIES )

    add_executable(test_parser
        test/test_parser.cpp)

    target_link_libraries(test_parser
        rosx_introspection)
endif()

if( COMPILING_WITH_AMENT )
    ament_package()
endif()

