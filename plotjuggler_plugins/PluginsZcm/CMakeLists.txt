
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
find_package(Zcm QUIET)

if(Zcm_FOUND)

  message(STATUS "[Zcm] found")

  include_directories(${Zcm_INCLUDE_DIR})

  # Streaming plugin
  qt5_wrap_ui(STREAM_UI datastream_zcm.ui config_zcm.ui)
  add_library(DataStreamZcm SHARED datastream_zcm.h datastream_zcm.cpp
                                   config_zcm.h config_zcm.cpp ${STREAM_UI})

  # log loading plugin
  target_link_libraries(DataStreamZcm PRIVATE Qt5::Widgets plotjuggler_base
                                              ${Zcm_LIBRARIES})

  target_compile_definitions(DataStreamZcm PRIVATE QT_PLUGIN)

  qt5_wrap_ui(LOAD_UI dataload_zcm.ui config_zcm.ui)
  add_library(DataLoadZcm SHARED dataload_zcm.h dataload_zcm.cpp config_zcm.h
                                 config_zcm.cpp ${LOAD_UI})

  target_link_libraries(DataLoadZcm PRIVATE Qt5::Widgets plotjuggler_base
                                            ${Zcm_LIBRARIES})

  install(TARGETS DataStreamZcm DataLoadZcm
          DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
  message("[Zcm] not found. Skipping plugin DataStreamZcma and DataLoadZcm.")
endif()
