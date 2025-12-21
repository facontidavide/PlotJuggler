function(find_or_download_lz4)

  if(TARGET LZ4::lz4_static)
    message(STATUS "LZ4 targets already defined")
    return()
  endif()

  find_package(LZ4 QUIET)

  # Check if LZ4 targets already exist (e.g., from Arrow)
  if(NOT TARGET LZ4::lz4_static)
    message(STATUS "Downloading and compiling LZ4")

    # lz4 ###
    cpmaddpackage(
      NAME lz4
      URL https://github.com/lz4/lz4/archive/refs/tags/v1.10.0.zip
      DOWNLOAD_ONLY YES)

    set(LZ4_FOUND TRUE FORCE)

    file(GLOB LZ4_SOURCES ${lz4_SOURCE_DIR}/lib/*.c)

    # define a helper to build both static and shared variants
    macro(build_lz4_variant TYPE SUFFIX)
      set(target lz4_${SUFFIX})
      add_library(${target} ${TYPE} ${LZ4_SOURCES})
      set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)

      add_library(LZ4::${target} INTERFACE IMPORTED)
      set_target_properties(LZ4::${target} PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${lz4_SOURCE_DIR}/lib
        INTERFACE_LINK_LIBRARIES ${target})
    endmacro()

    # now build both
    build_lz4_variant(STATIC static)

  endif()

endfunction()
