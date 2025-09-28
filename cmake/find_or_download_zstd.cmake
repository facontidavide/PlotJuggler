function(find_or_download_zstd)

  if(NOT TARGET zstd::libzstd_static)

    message(STATUS "Downloading and compiling ZSTD")

    # zstd ###
    cpmaddpackage(
      NAME zstd
      URL https://github.com/facebook/zstd/archive/refs/tags/v1.5.7.zip
      DOWNLOAD_ONLY YES)

    set(LIBRARY_DIR ${zstd_SOURCE_DIR}/lib)
    file(GLOB CommonSources ${LIBRARY_DIR}/common/*.c)
    file(GLOB CommonHeaders ${LIBRARY_DIR}/common/*.h)

    file(GLOB CompressSources ${LIBRARY_DIR}/compress/*.c)
    file(GLOB CompressHeaders ${LIBRARY_DIR}/compress/*.h)

    file(GLOB DecompressSources ${LIBRARY_DIR}/decompress/*.c)
    file(GLOB DecompressHeaders ${LIBRARY_DIR}/decompress/*.h)

    set(Sources ${CommonSources} ${CompressSources} ${DecompressSources})
    set(Headers ${PublicHeaders} ${CommonHeaders} ${CompressHeaders} ${DecompressHeaders})

    add_compile_options(-DZSTD_DISABLE_ASM)

    set(ZSTD_FOUND TRUE)

    add_library(libzstd_static STATIC ${Sources} ${Headers})
    set_property(TARGET libzstd_static PROPERTY POSITION_INDEPENDENT_CODE ON)

    add_library(zstd::libzstd_static INTERFACE IMPORTED)
    set_target_properties(zstd::libzstd_static
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${zstd_SOURCE_DIR}/lib
                 INTERFACE_LINK_LIBRARIES libzstd_static)

    set_target_properties(zstd::libzstd_static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${zstd_SOURCE_DIR}/lib
        INTERFACE_LINK_LIBRARIES libzstd_static)

  endif()

endfunction()
