find_package(Rlog)

if(Rlog_FOUND)
  include_directories(  ../ ${Rlog_INCLUDE_DIRS} )

  find_package(PythonInterp REQUIRED)

  add_definitions(${QT_DEFINITIONS})
  add_definitions(-DQT_PLUGIN)
  add_definitions(-DDYNAMIC_CAPNP)
  add_definitions(-DDBC_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/dbc")

  SET(SRC
    rlog_parser.cpp
    dataload_rlog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/dbc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/packer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/parser.cc
  )

  add_definitions(-L /usr/local/lib)

  add_library(DataLoadRlog SHARED ${SRC} ${UI_SRC}  )

  target_link_libraries(DataLoadRlog
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    ${Rlog_LIBRARIES} # has bz2, capnp, capnpc, kj
    plotjuggler_base)

  install(TARGETS DataLoadRlog DESTINATION bin  )

else()
  message("[Rlog] not found. Skipping plugin DataStreamRlog.")
endif()