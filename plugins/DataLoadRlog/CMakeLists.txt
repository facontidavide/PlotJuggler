include_directories(  ../ )

find_package(PythonInterp REQUIRED)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)
add_definitions(-DDYNAMIC_CAPNP)

add_custom_target(
  compile_dbcs ALL
)
file(GLOB dbc_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/*.dbc)
foreach(dbc ${dbc_FILES})
  string(REPLACE ".dbc" ".cc" dbc ${dbc})
  string(REPLACE "/3rdparty/opendbc/" "/3rdparty/opendbc/can/dbc_out/" dbc ${dbc})
  add_custom_command(
    TARGET compile_dbcs
    PRE_BUILD
    COMMAND ${PYTHON_EXECUTABLE} opendbc/can/process_dbc.py opendbc/ ${dbc}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/
    DEPENDS ${dbc}
  )
endforeach()

file(GLOB opendbc_out_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/dbc_out/*.cc)

SET(SRC
  rlog_parser.cpp
  dataload_rlog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/common.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/dbc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/packer.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/parser.cc
  ${opendbc_out_SRC}
)

add_library(DataLoadRlog SHARED ${SRC} ${UI_SRC}  )
add_dependencies(DataLoadRlog compile_dbcs  )

target_link_libraries(DataLoadRlog
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Xml_LIBRARIES}
  -lcapnp
  -lcapnpc
  -lbz2
  -lkj
  plotjuggler_plugin_base)

install(TARGETS DataLoadRlog DESTINATION bin  )
