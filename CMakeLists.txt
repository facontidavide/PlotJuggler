cmake_minimum_required(VERSION 3.16)

project(DataStreamWebSocket LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets WebSockets)

add_executable(DSWS_Test
  websocket_client.cpp
  websocket_client.ui
  websocket_client.h
  main.cpp
)

target_include_directories(DSWS_Test PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DSWS_Test PRIVATE
  Qt5::Core
  Qt5::Widgets
  Qt5::WebSockets
)

include(GNUInstallDirs)
install(TARGETS DSWS_Test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
