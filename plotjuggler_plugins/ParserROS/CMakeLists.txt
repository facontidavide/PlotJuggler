
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

SET(INTROSPECTION_TESTS OFF CACHE BOOL "Compile the tests")
add_subdirectory(rosx_introspection)

include_directories( rosx_introspection/include )

add_library(ParserROS1 SHARED
    ros_parser.cpp
    ros1_parser.h )

target_link_libraries(ParserROS1
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base)

add_library(ParserROS2 SHARED
    ros_parser.cpp
    ros2_parser.h )

target_link_libraries(ParserROS2
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base)

install(TARGETS ParserROS1 ParserROS2
    DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

