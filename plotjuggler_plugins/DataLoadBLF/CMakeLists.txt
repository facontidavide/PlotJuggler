if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.16)
  project(DataLoadBLFCore LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  find_package(Qt5 REQUIRED COMPONENTS Core Xml Widgets)
  find_package(vector_blf QUIET CONFIG)
  find_package(Vector_BLF QUIET CONFIG)
  if(NOT TARGET vector_blf::vector_blf AND TARGET Vector_BLF::Vector_BLF)
    add_library(vector_blf::vector_blf INTERFACE IMPORTED)
    set_target_properties(
      vector_blf::vector_blf
      PROPERTIES INTERFACE_LINK_LIBRARIES Vector_BLF::Vector_BLF)
  endif()
  if(NOT TARGET vector_blf::vector_blf)
    find_library(VECTOR_BLF_LIBRARY NAMES Vector_BLF)
    find_path(VECTOR_BLF_INCLUDE_DIR NAMES Vector/BLF/File.h)
    if(VECTOR_BLF_LIBRARY AND VECTOR_BLF_INCLUDE_DIR)
      add_library(vector_blf::vector_blf INTERFACE IMPORTED)
      set_target_properties(
        vector_blf::vector_blf
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VECTOR_BLF_INCLUDE_DIR}"
                   INTERFACE_LINK_LIBRARIES "${VECTOR_BLF_LIBRARY}")
    endif()
  endif()

  find_package(dbcppp QUIET CONFIG)
  if(NOT TARGET dbcppp::dbcppp AND TARGET dbcppp)
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES dbcppp)
  endif()
  if(NOT TARGET dbcppp::dbcppp)
    find_library(DBCPPP_LIBRARY NAMES dbcppp)
    find_path(DBCPPP_INCLUDE_DIR NAMES dbcppp/Network.h)
    if(DBCPPP_LIBRARY AND DBCPPP_INCLUDE_DIR)
      add_library(dbcppp::dbcppp INTERFACE IMPORTED)
      set_target_properties(dbcppp::dbcppp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DBCPPP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${DBCPPP_LIBRARY}")
    endif()
  endif()

  include(CTest)
endif()

add_library(blf_core STATIC blf_series_naming.cpp blf_config.cpp dbc_manager.cpp
                            blf_decoder.cpp)
target_compile_features(blf_core PUBLIC cxx_std_17)
target_include_directories(blf_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(blf_core PUBLIC Qt5::Core Qt5::Xml)

add_library(blf_adapters STATIC blf_reader.cpp dbcppp_decoder.cpp)
target_compile_features(blf_adapters PUBLIC cxx_std_17)
target_include_directories(blf_adapters PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(blf_adapters PUBLIC blf_core Qt5::Core)

if(TARGET vector_blf::vector_blf)
  target_compile_definitions(blf_adapters PRIVATE PJ_HAS_VECTOR_BLF=1)
  target_link_libraries(blf_adapters PRIVATE vector_blf::vector_blf)
else()
  target_compile_definitions(blf_adapters PRIVATE PJ_HAS_VECTOR_BLF=0)
endif()

if(TARGET dbcppp::dbcppp)
  target_compile_definitions(blf_adapters PRIVATE PJ_HAS_DBCPPP=1)
  target_link_libraries(blf_adapters PRIVATE dbcppp::dbcppp)
else()
  target_compile_definitions(blf_adapters PRIVATE PJ_HAS_DBCPPP=0)
endif()

if(TARGET plotjuggler_base)
  qt5_wrap_ui(UI_SRC dialog_blf.ui)

  add_library(DataLoadBLF SHARED dataload_blf.cpp dialog_blf.cpp ${UI_SRC})
  target_compile_features(DataLoadBLF PRIVATE cxx_std_17)
  target_include_directories(DataLoadBLF PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(DataLoadBLF PRIVATE Qt5::Core Qt5::Xml Qt5::Widgets plotjuggler_base
                                            blf_core blf_adapters)
  target_compile_definitions(DataLoadBLF PRIVATE QT_PLUGIN)

  install(TARGETS DataLoadBLF DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
endif()

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  option(BLF_BUILD_E2E_RUNNER "Build BLF end-to-end validation runner" ON)
else()
  option(BLF_BUILD_E2E_RUNNER "Build BLF end-to-end validation runner" OFF)
endif()

if(BLF_BUILD_E2E_RUNNER)
  add_executable(blf_e2e_runner tools/blf_e2e_runner.cpp)
  target_compile_features(blf_e2e_runner PRIVATE cxx_std_17)
  target_link_libraries(blf_e2e_runner PRIVATE blf_core blf_adapters Qt5::Core Qt5::Xml)
endif()

if(BUILD_TESTING)
  find_package(GTest QUIET)
  if(GTest_FOUND)
    include(GoogleTest)

    function(add_blf_test test_name)
      add_executable(${test_name} tests/${test_name}.cpp)
      target_link_libraries(${test_name} PRIVATE blf_core GTest::gtest_main Qt5::Core
                                             Qt5::Xml)
      gtest_discover_tests(${test_name})
    endfunction()

    add_blf_test(test_blf_series_naming)
    add_blf_test(test_blf_config)
    add_blf_test(test_dbc_manager)
    add_blf_test(test_blf_decoder)
    add_executable(test_blf_reader_time tests/test_blf_reader_time.cpp)
    target_link_libraries(test_blf_reader_time PRIVATE blf_adapters GTest::gtest_main Qt5::Core
                                                       Qt5::Xml)
    gtest_discover_tests(test_blf_reader_time)
  else()
    message(STATUS "GTest not found, skipping DataLoadBLF unit tests")
  endif()
endif()
