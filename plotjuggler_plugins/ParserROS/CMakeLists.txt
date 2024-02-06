
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

SET(INTROSPECTION_TESTS OFF CACHE BOOL "Compile the tests")

# Check if default fastCdr library generation is needed
find_package(fastcdr QUIET)

if(NOT fastcdr_FOUND )
    message(STATUS "[FastCdr] not found, create shared libraries")
    # Override Fast-CDR option: compile as static lib
    SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Create shared libraries by default")
    add_subdirectory(rosx_introspection/3rdparty/Fast-CDR)
    include_directories(rosx_introspection/3rdparty/Fast-CDR/include)
else()
    # Mention that FastCdr has been found on system
    message(STATUS "[FastCdr] found, version: ${fastcdr_VERSION}")
endif()

add_subdirectory(rosx_introspection)

include_directories( rosx_introspection/include )

add_library(ParserROS1 SHARED
    special_messages.cpp
    ros_parser.cpp
    ros1_parser.h )

target_link_libraries(ParserROS1
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base)

add_library(ParserROS2 SHARED
    special_messages.cpp
    ros_parser.cpp
    ros2_parser.h )

target_link_libraries(ParserROS2
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base)

install(TARGETS ParserROS1 ParserROS2
    DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

