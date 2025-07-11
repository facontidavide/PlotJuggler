if(BUILDING_WITH_CONAN)
  message(STATUS "Finding Protobuf with conan")
  set(Protobuf_LIBS protobuf::libprotobuf)
else()
  message(STATUS "Finding Protobuf without package managers")
  find_package(Protobuf QUIET)
  set(Protobuf_LIBS ${Protobuf_LIBRARIES})
endif()

find_package(Protobuf QUIET)

if(Protobuf_FOUND)

  include_directories(../)
  message(STATUS "[Protobuf] found")
  qt5_wrap_ui(UI_SRC protobuf_parser.ui)

  include_directories(${Protobuf_INCLUDE_DIRS})

  add_library(
    ProtobufParser SHARED error_collectors.cpp protobuf_parser.cpp
                          protobuf_factory.cpp protobuf_parser.h ${UI_SRC})

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(ABSL_SPINLOCK_WAIT_LIB absl_spinlock_wait)

    target_link_libraries(ProtobufParser PRIVATE ${ABSL_SPINLOCK_WAIT_LIB})

    target_compile_definitions(ProtobufParser PRIVATE QT_PLUGIN)
  endif()

  target_link_libraries(
    ProtobufParser PRIVATE Qt5::Widgets Qt5::Xml ${Protobuf_LIBS}
                           plotjuggler_base plotjuggler_qwt fmt::fmt)

  install(TARGETS ProtobufParser DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
  message("[Protobuf] not found: skipping compilation of plugin ProtobufParser")
endif()
