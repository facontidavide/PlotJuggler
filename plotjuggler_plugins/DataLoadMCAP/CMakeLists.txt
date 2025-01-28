add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )


########################################

# Still have some problem setting up CPM for this...
find_package(zstd QUIET)
if (NOT zstd_FOUND)
    CPMAddPackage(
        NAME zstd
        GITHUB_REPOSITORY facebook/zstd
        GIT_TAG v1.5.5
        DOWNLOAD_ONLY YES
    )
    # zstd has no CMake support, so we create our own target
    if (zstd_ADDED)
        file(GLOB ZSTD_SOURCES ${zstd_SOURCE_DIR}/lib/*.c)
        add_library(libzstd_static STATIC ${ZSTD_SOURCES})
        target_include_directories(libzstd_static PUBLIC ${zstd_SOURCE_DIR}/lib)
    endif()
endif()
########################################
find_package(lz4 QUIET)
if (NOT lz4_FOUND)
    CPMAddPackage(
        NAME lz4
        GITHUB_REPOSITORY lz4/lz4
        GIT_TAG v1.10.0
        DOWNLOAD_ONLY YES
    )
    if (lz4_ADDED)
        file(GLOB LZ4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)
        add_library(lz4_static STATIC ${LZ4_SOURCES})
        target_include_directories(lz4_static PUBLIC ${lz4_SOURCE_DIR}/lib)
    endif()
endif()

########################################
CPMAddPackage(
    NAME mcap
    GITHUB_REPOSITORY foxglove/mcap
    GIT_TAG releases/cpp/v2.0.0
    DOWNLOAD_ONLY YES
)
# mcap has no CMake support, so we create our own target
add_library(mcap INTERFACE)
target_include_directories(mcap INTERFACE "${mcap_SOURCE_DIR}/cpp/mcap/include")
target_link_libraries(mcap INTERFACE libzstd_static lz4_static)
########################################
add_library(DataLoadMCAP SHARED
    mcap.cpp
    dataload_mcap.cpp
    dialog_mcap.cpp
    ${UI_SRC}  )

target_link_libraries(DataLoadMCAP PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    mcap
    data_tamer_parser
    plotjuggler_base )

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
