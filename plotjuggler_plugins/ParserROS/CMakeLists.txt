
add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

SET(INTROSPECTION_TESTS OFF CACHE BOOL "Compile the tests")

add_subdirectory(rosx_introspection)

add_library(ParserROS1 SHARED
    special_messages.cpp
    ros_parser.cpp
    ros1_parser.h )

target_include_directories(rosx_introspection PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rosx_introspection/include>
    )

target_link_libraries(ParserROS1
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base
    data_tamer::parser)

add_library(ParserROS2 SHARED
    special_messages.cpp
    ros_parser.cpp
    ros2_parser.h )

target_link_libraries(ParserROS2
    ${Qt5Widgets_LIBRARIES}
    rosx_introspection
    plotjuggler_base
    data_tamer::parser)

install(TARGETS ParserROS1 ParserROS2
    DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
