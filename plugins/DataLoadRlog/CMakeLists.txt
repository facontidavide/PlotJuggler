include_directories(  ../ )

find_package(PythonInterp REQUIRED)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DDYNAMIC_CAPNP)

SET(SRC
  rlog_parser.cpp
  dataload_rlog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/common.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/dbc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/packer.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/parser.cc
)

add_definitions(-L /usr/local/lib)

add_custom_target(
  compile_dbcs ALL
)
file(GLOB dbc_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/*.dbc)
foreach(dbc ${dbc_FILES})
  string(REPLACE ".dbc" ".cc" dbc_out ${dbc})
  string(REPLACE "/3rdparty/opendbc/" "/3rdparty/opendbc/can/dbc_out/" dbc_out ${dbc_out})
  set(SRC ${SRC} ${dbc_out})
  add_custom_command(
    PRE_BUILD
    COMMAND ${PYTHON_EXECUTABLE} opendbc/can/process_dbc.py opendbc/ ${dbc_out}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/
    DEPENDS ${dbc}
    OUTPUT ${dbc_out}
  )
endforeach()

add_library(DataLoadRlog SHARED ${SRC} ${UI_SRC}  )
add_dependencies(DataLoadRlog compile_dbcs  )

target_link_libraries(DataLoadRlog
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Xml_LIBRARIES}
  bz2.a
  capnpc.a
  capnp.a
  kj.a
  plotjuggler_plugin_base)

install(TARGETS DataLoadRlog DESTINATION bin  )
