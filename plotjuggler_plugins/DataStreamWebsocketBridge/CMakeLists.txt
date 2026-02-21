cmake_minimum_required(VERSION 3.16)
project(DataStreamWebsocketBridge)

find_package(Qt5 REQUIRED COMPONENTS Widgets WebSockets Xml)

find_path(ZSTD_INCLUDE_DIR zstd.h)
find_library(ZSTD_LIBRARY NAMES zstd libzstd)

if(NOT Qt5WebSockets_FOUND)
  message(STATUS "Qt5WebSockets not found: disabling DataStreamWebsocketBridge")
  return()
endif()

if(NOT ZSTD_INCLUDE_DIR OR NOT ZSTD_LIBRARY)
  message(STATUS "ZSTD not found: disabling DataStreamWebsocketBridge")
  return()
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(../)

qt5_wrap_ui(UI_SRC
  websocket_client.ui
)

set(SRC
  websocket_client.cpp
)

add_library(DataStreamWebsocketBridge SHARED
  ${SRC}
  ${UI_SRC}
)

target_include_directories(DataStreamWebsocketBridge PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${ZSTD_INCLUDE_DIR}
)

target_link_libraries(DataStreamWebsocketBridge PRIVATE
  Qt5::Widgets
  Qt5::WebSockets
  Qt5::Xml
  plotjuggler_base
  ${ZSTD_LIBRARY}
)

target_compile_definitions(DataStreamWebsocketBridge PRIVATE
  QT_PLUGIN
  PJ_BUILD
)

install(TARGETS DataStreamWebsocketBridge
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
