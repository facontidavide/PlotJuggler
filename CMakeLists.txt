cmake_minimum_required(VERSION 3.16)

project(DataStreamWebsocketBridge)

find_package(Qt5 REQUIRED COMPONENTS Widgets WebSockets)

qt5_wrap_ui(UI_SRC websocket_client.ui)

add_library(DataStreamWebsocketBridge SHARED
  websocket_client.cpp
  ${UI_SRC}
)

target_include_directories(DataStreamWebsocketBridge PRIVATE
  ../
)

target_link_libraries(DataStreamWebsocketBridge PRIVATE
  Qt5::Widgets
  Qt5::WebSockets
  plotjuggler_base
  zstd
)

target_compile_definitions(DataStreamWebsocketBridge PRIVATE
  QT_PLUGIN
  PJ_BUILD
)

install(TARGETS DataStreamWebsocketBridge
        DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

