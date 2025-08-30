
find_package(Protobuf QUIET)

if(Protobuf_FOUND)

  include_directories(../)

  if(TARGET protobuf::protobuf)
    message(STATUS "[Protobuf] found (Conan?)")
    SET(PROTOBUF_DEPS protobuf::protobuf)
  elseif(TARGET protobuf::libprotobuf)
      message(STATUS "[Protobuf] found")
      SET(PROTOBUF_DEPS protobuf::libprotobuf)
  endif()

  qt5_wrap_ui(UI_SRC protobuf_parser.ui)

  add_library(
    ProtobufParser SHARED
    error_collectors.cpp
    protobuf_parser.cpp
    protobuf_factory.cpp
    protobuf_parser.h
    ${UI_SRC})

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(ABSL_SPINLOCK_WAIT_LIB absl_spinlock_wait)
    target_link_libraries(ProtobufParser PRIVATE ${ABSL_SPINLOCK_WAIT_LIB})
    target_compile_definitions(ProtobufParser PRIVATE QT_PLUGIN)
  endif()

  target_link_libraries(ProtobufParser PRIVATE
    Qt5::Widgets
    Qt5::Xml
    ${PROTOBUF_DEPS}
    plotjuggler_base
    plotjuggler_qwt
    fmt::fmt)

  install(TARGETS ProtobufParser DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
  message("[Protobuf] not found: skipping compilation of plugin ProtobufParser")
endif()
