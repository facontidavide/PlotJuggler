qt5_wrap_ui(UI_SRC dialog_mcap.ui)

if(COMPILING_WITH_AMENT)
  # This is the ROS package
  message(STATUS "Found mcap_vendor (ROS2): ${mcap_vendor_VERSION}")
  find_package(mcap_vendor REQUIRED)
  set(MCAP_DEPS mcap_vendor::mcap)
else()

  find_package(mcap QUIET CONFIG)

  if(mcap_FOUND)
    message(STATUS "mcap found (conan?)")
    set(MCAP_DEPS mcap::mcap)

  elseif(NOT TARGET mcap::mcap AND NOT TARGET mcap_static)

    include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_lz4.cmake)
    find_or_download_lz4()

    include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_zstd.cmake)
    find_or_download_zstd()

    # mcap
    cpmaddpackage(
      NAME mcap
      GITHUB_REPOSITORY foxglove/mcap
      GIT_TAG releases/cpp/v1.4.1
      DOWNLOAD_ONLY YES)
    # mcap has no CMake support, so we create our own target

    add_library(mcap_static STATIC mcap.cpp)

    target_include_directories(mcap_static PUBLIC
       $<BUILD_INTERFACE:${mcap_SOURCE_DIR}/cpp/mcap/include>
    )

    target_link_libraries(mcap_static PUBLIC
        LZ4::lz4_static zstd::libzstd_static)

    set(MCAP_DEPS mcap_static)
  endif()
endif()

# ##############################################################################

add_library(DataLoadMCAP SHARED dataload_mcap.cpp dialog_mcap.cpp ${UI_SRC})

target_link_libraries(DataLoadMCAP
   PRIVATE
    Qt5::Widgets
    Qt5::Xml
    Qt5::Concurrent
    plotjuggler_base
    ${MCAP_DEPS})

# Suppress LNK4217 warnings on Windows for MCAP static library symbols
if(WIN32 AND MSVC)
  target_link_options(DataLoadMCAP PRIVATE /ignore:4217)
endif()

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
