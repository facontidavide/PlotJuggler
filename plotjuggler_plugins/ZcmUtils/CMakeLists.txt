if(BUILDING_WITH_VCPKG)
  message(STATUS "Finding Zcm with vcpkg")
  set(Zcm_LIBRARIES libzcm)
elseif(BUILDING_WITH_CONAN)
  message(STATUS "Finding Zcm with conan")
  set(Zcm_LIBRARIES libzcm)
else()
  message(STATUS "Finding Zcm without package managers")
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
endif()

find_package(Zcm QUIET)

if(Zcm_FOUND)

  message(STATUS "[Zcm] found")

  include_directories(${Zcm_INCLUDE_DIR})

  qt5_wrap_ui(UI_SRC datastream_zcm.ui)

  set(SRC datastream_zcm.h datastream_zcm.cpp)

  add_library(DataStreamZcm SHARED ${SRC} ${UI_SRC})

  target_link_libraries(DataStreamZcm PRIVATE Qt5::Widgets plotjuggler_base
                                              ${Zcm_LIBRARIES})

  target_compile_definitions(DataStreamZcm PRIVATE QT_PLUGIN)

  install(TARGETS DataStreamZcm DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
else()
  message("[Zcm] not found. Skipping plugin DataStreamZcm.")
endif()
