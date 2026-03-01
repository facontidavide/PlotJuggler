cmake_minimum_required(VERSION 3.16)
project(DataStreamWebsocketBridge)

find_package(Qt5 REQUIRED COMPONENTS Widgets WebSockets Xml)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(../)

qt5_wrap_ui(UI_SRC
  websocket_client.ui
)

set(SRC
  websocket_client.cpp
  websocket_client_config.cpp
  websocket_dialog.cpp
)

add_library(DataStreamWebsocketBridge SHARED
  ${SRC}
  ${UI_SRC}
)

target_include_directories(DataStreamWebsocketBridge PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DataStreamWebsocketBridge PRIVATE
  Qt5::Widgets
  Qt5::WebSockets
  Qt5::Xml
  plotjuggler_base
  zstd::libzstd_static
)

target_compile_definitions(DataStreamWebsocketBridge PRIVATE
  QT_PLUGIN
  PJ_BUILD
)

install(TARGETS DataStreamWebsocketBridge
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
