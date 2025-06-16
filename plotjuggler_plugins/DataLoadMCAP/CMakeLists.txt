add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )

find_package(mcap_vendor QUIET)

if(mcap_vendor_FOUND)
    message(STATUS "Found mcap_vendor: ${mcap_vendor_VERSION}")
    add_library(mcap::mcap ALIAS mcap_vendor::mcap)
else()
    message(STATUS "mcap_vendor not found. Will be downloaded with CPM.")
    if(NOT TARGET mcap::mcap)

        include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_lz4.cmake)
        find_or_download_lz4()

        include(${PROJECT_SOURCE_DIR}/cmake/find_or_download_zstd.cmake)
        find_or_download_zstd()

        ### mcap ###
        CPMAddPackage(
            NAME mcap
            GITHUB_REPOSITORY foxglove/mcap
            GIT_TAG releases/cpp/v1.3.0
            DOWNLOAD_ONLY YES
        )
        # mcap has no CMake support, so we create our own target
        add_library(mcap_lib STATIC mcap.cpp)
        target_include_directories(mcap_lib
             PUBLIC $<BUILD_INTERFACE:${mcap_SOURCE_DIR}/cpp/mcap/include>
        )
        target_link_libraries(mcap_lib
            PUBLIC
                lz4::lz4
                zstd::zstd
        )
        add_library(mcap::mcap ALIAS mcap_lib)
    endif()
endif()


########################################


add_library(DataLoadMCAP SHARED
    dataload_mcap.cpp
    dialog_mcap.cpp
    ${UI_SRC}  )

target_link_libraries(DataLoadMCAP PRIVATE
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        ${Qt5Concurrent_LIBRARIES}
        plotjuggler_base
        mcap::mcap

    )

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
