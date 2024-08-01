find_package(Cereal)
find_package(Rlog)

if(Cereal_FOUND AND Rlog_FOUND)
  include_directories(../ ${Rlog_INCLUDE_DIRS})

  add_definitions(${QT_DEFINITIONS})
  add_definitions(-DQT_PLUGIN)
  add_definitions(-DDYNAMIC_CAPNP)
  add_definitions(-DDBC_FILE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/dbc")

  QT5_WRAP_UI ( UI_SRC  datastream_cereal.ui  )

  SET(SRC
    datastream_cereal.cpp
    ../DataLoadRlog/rlog_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/common.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/dbc.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/packer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/opendbc/can/parser.cc
  )

  add_library(DataStreamCereal SHARED ${SRC} ${UI_SRC}  )

  target_link_libraries(DataStreamCereal
      ${Qt5Widgets_LIBRARIES}
      ${Cereal_LIBRARIES}  # has libcereal.a and libmessaging.a
      ${Rlog_LIBRARIES} # has bz2, capnp, kj
      plotjuggler_base
  )

else()
  message("[Cereal] not found. Skipping plugin DataStreamCereal.")
endif()