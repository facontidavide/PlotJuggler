find_package(Qt5 QUIET COMPONENTS WebSockets)

if(NOT Qt5WebSockets_FOUND)
  message(
    STATUS
      "Can't find Qt5WebSockets in your system. Have you tried [sudo apt-get install libqt5websockets5-dev] ?"
  )
else()

  include_directories(../)

  qt5_wrap_ui(UI_SRC websocket_server.ui)

  set(SRC websocket_server.cpp)

  add_library(DataStreamWebSocket SHARED ${SRC} ${UI_SRC})

  target_link_libraries(DataStreamWebSocket
                        PRIVATE Qt5::Widgets Qt5::WebSockets plotjuggler_base)

  target_compile_definitions(DataStreamWebSocket PRIVATE QT_PLUGIN)

  install(TARGETS DataStreamWebSocket
          DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

endif()
