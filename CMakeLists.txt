cmake_minimum_required(VERSION 3.16)

project(DataStreamWebSocket LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Widgets)

add_executable(DataStreamWebSocket
  websocket_client.cpp
  websocket_client.ui
  websocket_client.h
  main.cpp
)
target_include_directories(DataStreamWebSocket PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DataStreamWebSocket PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::WebSockets
)

include(GNUInstallDirs)
install(TARGETS DataStreamWebSocket
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
