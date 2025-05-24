function(find_or_download_zstd)

    find_package(ZSTD QUIET)

    if (ZSTD_FOUND)
        message(STATUS "Found ZSTD in system")

        if(NOT TARGET zstd::zstd)
            add_library(zstd::zstd INTERFACE IMPORTED)
            set_target_properties(zstd::zstd PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${ZSTD_LIBRARY}"
            )
        endif()

    elseif(NOT TARGET libzstd_static)
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
        set(ZSTD_FOUND TRUE)

        add_library(zstd::zstd INTERFACE IMPORTED)
        set_target_properties(zstd::zstd PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${zstd_SOURCE_DIR}/lib
            INTERFACE_LINK_LIBRARIES libzstd_static
        )
    endif()

    if(NOT TARGET zstd::zstd)
        message(FATAL_ERROR "ZSTD not found, please install ZSTD or download it")
    endif()

endfunction()
