set(INTROSPECTION_TESTS
    OFF
    CACHE BOOL "Compile the tests")

add_subdirectory(rosx_introspection)

add_library(ParserROS1 SHARED special_messages.cpp ros_parser.cpp ros1_parser.h)

target_include_directories(
  rosx_introspection
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/rosx_introspection/include>)

target_link_libraries(
  ParserROS1 PRIVATE Qt5::Widgets rosx_introspection plotjuggler_base
                     data_tamer::parser fmt::fmt)

target_compile_definitions(ParserROS1 PRIVATE QT_PLUGIN)

add_library(ParserROS2 SHARED special_messages.cpp ros_parser.cpp ros2_parser.h)

target_link_libraries(
  ParserROS2 PRIVATE Qt5::Widgets rosx_introspection plotjuggler_base
                     data_tamer::parser fmt::fmt)

install(TARGETS ParserROS1 ParserROS2
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
