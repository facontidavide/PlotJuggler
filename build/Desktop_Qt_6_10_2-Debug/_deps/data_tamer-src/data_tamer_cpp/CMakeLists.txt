cmake_minimum_required(VERSION 3.16)

project(data_tamer_cpp VERSION 1.0.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    option(DATA_TAMER_BUILD_TESTS "Build tests" ON)
    option(DATA_TAMER_BUILD_EXAMPLES "Build examples" ON)
else()
    option(DATA_TAMER_BUILD_TESTS "Build tests" OFF)
    option(DATA_TAMER_BUILD_EXAMPLES "Build examples" OFF)
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

# optionally build for ROS 2
option(DATA_TAMER_BUILD_ROS "Build for ROS 2" ON)
if (DATA_TAMER_BUILD_ROS)
    find_package(ament_cmake QUIET)
    if (NOT ament_cmake_FOUND)
        set(DATA_TAMER_BUILD_ROS FALSE)
        message(WARNING "ament_cmake not found, building without ROS 2 support. Set DATA_TAMER_BUILD_ROS to false to disable this warning.")
    endif()
endif()

###########################################
# check if mcap can be found. If true,
# probably we used conan. If false, build from 3rdparty
find_package(mcap QUIET)
if(NOT mcap_FOUND AND NOT DATA_TAMER_BUILD_ROS)
    set(USE_VENDORED_MCAP ON)
    message(STATUS "MCAP from 3rdparty")
    add_subdirectory(3rdparty/mcap)
    set(mcap_LIBRARY mcap_lib)
else()
    message(STATUS "MCAP provided by conan (?)")
    set(mcap_LIBRARY mcap::mcap)
endif()
###########################################


if (DATA_TAMER_BUILD_ROS)
    set(ROS2_SINK src/sinks/ros2_publisher_sink.cpp)
endif()

if(BUILD_SHARED_LIBS OR DATA_TAMER_BUILD_ROS)
    set(LIB_TYPE SHARED)
else()
    set(LIB_TYPE STATIC)
endif()

if(DATA_TAMER_BUILD_ROS AND NOT BUILD_SHARED_LIBS)
    message("BUILD_SHARED_LIBS forced to ON (i.e. SHARED) in ROS2. Set BUILD_SHARED_LIBS to true to disable this warning when building with ROS.")
endif()

add_library(data_tamer ${LIB_TYPE}
    include/data_tamer/channel.hpp
    include/data_tamer/custom_types.hpp
    include/data_tamer/data_tamer.hpp
    include/data_tamer/types.hpp
    include/data_tamer/values.hpp
    include/data_tamer/sinks/dummy_sink.hpp
    include/data_tamer/sinks/mcap_sink.hpp

    src/channel.cpp
    src/data_tamer.cpp
    src/data_sink.cpp
    src/types.cpp

    src/sinks/mcap_sink.cpp
    ${ROS2_SINK}

    include/data_tamer/logged_value.hpp
)


if (MSVC)
    target_compile_options(data_tamer PRIVATE /W4 /WX)
else()
    target_compile_options(data_tamer PRIVATE -Wall -Wconversion -Wextra -Wsign-conversion -Werror -Wpedantic -Wno-sign-conversion)
endif()

target_compile_features(data_tamer PUBLIC cxx_std_17)
target_include_directories(data_tamer
 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
 PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>
)
target_compile_definitions(data_tamer PUBLIC -DDATA_TAMER_VERSION="${CMAKE_PROJECT_VERSION}")

set(INSTALL_TARGETS data_tamer)

if (DATA_TAMER_BUILD_ROS)
    message(STATUS "Compiling for ROS2")
    target_compile_definitions(data_tamer PUBLIC USING_ROS2=1 )

    find_package(mcap_vendor REQUIRED)
    find_package(rclcpp REQUIRED)
    find_package(rclcpp_lifecycle REQUIRED)
    find_package(data_tamer_msgs REQUIRED)

    target_include_directories(data_tamer PUBLIC
        ${mcap_vendor_INCLUDE_DIRS}
        ${rclcpp_INCLUDE_DIRS}
        ${rclcpp_lifecycle_INCLUDE_DIRS}
        ${data_tamer_msgs_INCLUDE_DIRS}
    )

    # we need to find the underlying library for `target_link_libraries` to work
    find_library(MCAP_LIBRARY mcap PATHS ${mcap_vendor_LIBRARY_DIRS})

    target_link_libraries(data_tamer PUBLIC
        ${mcap_vendor_LIBRARIES}
        ${rclcpp_LIBRARIES}
        ${rclcpp_lifecycle_LIBRARIES}
        ${data_tamer_msgs_LIBRARIES}
        ${MCAP_LIBRARY}
    )

    ament_export_targets(data_tamerTargets HAS_LIBRARY_TARGET)
    ament_export_dependencies(mcap_vendor rclcpp rclcpp_lifecycle data_tamer_msgs)
    ament_package()

elseif( USE_VENDORED_MCAP )
    target_include_directories(data_tamer PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mcap/include> )
    target_link_libraries(data_tamer PRIVATE mcap_lib)
    list(APPEND INSTALL_TARGETS mcap_lib)
else()
    find_package(mcap REQUIRED)
    target_link_libraries(data_tamer PRIVATE mcap::mcap)
endif()

install(
  DIRECTORY include/
  DESTINATION include
)


install(
  TARGETS ${INSTALL_TARGETS}
  EXPORT data_tamerTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

if (NOT DATA_TAMER_BUILD_ROS)

    set(LIBRARY_INSTALL_DIR lib)
    set(INCLUDE_INSTALL_DIR include)

    # Install config file for finding the package.
    include(CMakePackageConfigHelpers)

    configure_package_config_file(
      ${CMAKE_CURRENT_SOURCE_DIR}/data_tamerConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/data_tamerConfig.cmake
      INSTALL_DESTINATION lib/cmake/data_tamer
      PATH_VARS INCLUDE_INSTALL_DIR)

    write_basic_package_version_file(
        "data_tamerConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(EXPORT data_tamerTargets
            FILE data_tamerTargets.cmake
            NAMESPACE data_tamer::
            DESTINATION lib/cmake/data_tamer )

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/data_tamerConfig.cmake"
            DESTINATION lib/cmake/data_tamer )

endif()

if(DATA_TAMER_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if(DATA_TAMER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

find_package(benchmark QUIET)
if(benchmark_FOUND)
    add_subdirectory(benchmarks)
else()
    message("Google Benchmark library not found")
endif()
