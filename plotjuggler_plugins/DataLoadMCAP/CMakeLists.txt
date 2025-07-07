qt5_wrap_ui(UI_SRC dialog_mcap.ui)

# This is the ROS package
find_package(mcap_vendor QUIET)

if(mcap_vendor_FOUND)
  message(STATUS "Found mcap_vendor: ${mcap_vendor_VERSION}")
  add_library(mcap::mcap ALIAS mcap_vendor::mcap)
else()
  message(STATUS "mcap_vendor not found. Will be downloaded with CPM.")
  if(NOT TARGET mcap::mcap AND NOT TARGET mcap_lib)

    include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_lz4.cmake)
    find_or_download_lz4()

    include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_zstd.cmake)
    find_or_download_zstd()

    # mcap ###
    cpmaddpackage(
      NAME
      mcap
      GITHUB_REPOSITORY
      foxglove/mcap
      GIT_TAG
      releases/cpp/v1.3.0
      DOWNLOAD_ONLY
      YES)
    # mcap has no CMake support, so we create our own target

    add_library(mcap_lib STATIC mcap.cpp)
    target_include_directories(
      mcap_lib PUBLIC $<BUILD_INTERFACE:${mcap_SOURCE_DIR}/cpp/mcap/include>)
    target_link_libraries(mcap_lib lz4::lz4 zstd::zstd)

    add_library(mcap::mcap ALIAS mcap_lib)
  endif()
endif()

# ##############################################################################

add_library(DataLoadMCAP SHARED dataload_mcap.cpp dialog_mcap.cpp ${UI_SRC})

target_link_libraries(
  DataLoadMCAP PRIVATE Qt5::Widgets Qt5::Xml Qt5::Concurrent plotjuggler_base
                       mcap::mcap)

# Suppress LNK4217 warnings on Windows for MCAP static library symbols
if(WIN32 AND MSVC)
  target_link_options(DataLoadMCAP PRIVATE /ignore:4217)
endif()

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
