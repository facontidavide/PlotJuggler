add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )


########################################
if(NOT TARGET mcap)
    ### zstd ###
    CPMAddPackage(
        NAME zstd
        GITHUB_REPOSITORY facebook/zstd
        GIT_TAG v1.5.6
        DOWNLOAD_ONLY YES
    )
    set(ZSTD_BUILD_STATIC ON  CACHE BOOL " " FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL " " FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF   CACHE BOOL " " FORCE)
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL " " FORCE)
    set(ZSTD_BUILD_TESTS OFF   CACHE BOOL " " FORCE)
    set(ZSTD_BUILD_CONTRIB OFF  CACHE BOOL " " FORCE)
    set(ZSTD_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
    set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_ZLIB_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_LZ4_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_LZMA_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_ZDICT_SUPPORT OFF CACHE BOOL " " FORCE)
    set(ZSTD_PROGRAMS "" CACHE STRING " " FORCE)

    add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})

    ### lz4 ###
    CPMAddPackage(
        NAME lz4
        GITHUB_REPOSITORY lz4/lz4
        GIT_TAG v1.10.0
        DOWNLOAD_ONLY YES
    )
    file(GLOB LZ4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)
    add_library(lz4_static STATIC ${LZ4_SOURCES})
    target_include_directories(lz4_static PUBLIC ${lz4_SOURCE_DIR}/lib)

    ### mcap ###
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
 endif()
########################################


add_library(DataLoadMCAP SHARED
    mcap.cpp
    dataload_mcap.cpp
    dialog_mcap.cpp
    ${UI_SRC}  )

target_include_directories(DataLoadMCAP PRIVATE
    $<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib>
    $<BUILD_INTERFACE:${lz4_SOURCE_DIR}/lib>)

target_link_libraries(DataLoadMCAP PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    mcap
    plotjuggler_base )

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
