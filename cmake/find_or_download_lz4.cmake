function(find_or_download_lz4)

    find_package(LZ4 QUIET)

    if (LZ4_FOUND)
        message(STATUS "Found LZ4 in system")

        if(NOT TARGET lz4::lz4)
            add_library(lz4::lz4 INTERFACE IMPORTED)
            set_target_properties(lz4::lz4 PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${LZ4_LIBRARY}"
            )
        endif()
    elseif(NOT TARGET lz4_static)
        ### lz4 ###
        CPMAddPackage(
            NAME lz4
            URL https://github.com/lz4/lz4/archive/refs/tags/v1.10.0.zip
            DOWNLOAD_ONLY YES
        )
        file(GLOB LZ4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)
        add_library(lz4_static STATIC ${LZ4_SOURCES})
        target_include_directories(lz4_static PUBLIC ${lz4_SOURCE_DIR}/lib)

        set(LZ4_FOUND TRUE)

        add_library(lz4::lz4 INTERFACE IMPORTED)
        set_target_properties(lz4::lz4 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${lz4_SOURCE_DIR}/lib
            INTERFACE_LINK_LIBRARIES lz4_static
        )
    endif()

    if(NOT TARGET lz4::lz4)
        message(FATAL_ERROR "LZ4 not found, please install LZ4 or download it")
    endif()

endfunction()
