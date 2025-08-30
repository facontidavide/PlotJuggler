find_package(ZeroMQ QUIET)

if(ZeroMQ_FOUND)
  message(STATUS "[ZeroMQ] found")

  qt5_wrap_ui(UI_SRC datastream_zmq.ui)

  add_library(DataStreamZMQ SHARED datastream_zmq.cpp ${UI_SRC})

  target_include_directories(DataStreamZMQ PRIVATE ${ZeroMQ_INCLUDE_DIRS})
  target_link_libraries(DataStreamZMQ PRIVATE Qt5::Widgets plotjuggler_base
                                              ${ZeroMQ_LIBRARIES})

  target_compile_definitions(DataStreamZMQ PRIVATE QT_PLUGIN)

  if(TARGET libzmq-static)
    target_link_libraries(DataStreamZMQ PRIVATE libzmq-static)
  else()
    target_link_libraries(DataStreamZMQ PRIVATE ${ZeroMQ_LIBRARIES})
  endif()

  install(TARGETS DataStreamZMQ DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
  message(WARNING "[ZeroMQ] not found. Skipping plugin DataStreamZMQ.")
endif()
