function(find_or_download_zstd)

  if(TARGET zstd::zstd)
    message(STATUS "ZSTD targets already defined")
    return()
  endif()

  find_package(ZSTD QUIET)

  # Determine which library type to use
  if(PREFER_DYNAMIC_ZSTD)
    set(ZSTD_PREFER_SHARED ON)
    message(STATUS "ZSTD: Preferring dynamic library")
  else()
    set(ZSTD_PREFER_SHARED OFF)
    message(STATUS "ZSTD: Preferring static library")
  endif()

  # Reuse any existing ZSTD targets from find_package or toolchains
  set(_zstd_base_candidates zstd::zstd ZSTD::ZSTD zstd)
  foreach(_candidate IN LISTS _zstd_base_candidates)
    if(TARGET ${_candidate})
      add_library(zstd::zstd ALIAS ${_candidate})
      message(STATUS "ZSTD: Using existing target ${_candidate}")
      return()
    endif()
  endforeach()

  set(_zstd_shared_candidates zstd::libzstd_shared ZSTD::libzstd_shared zstd_shared)
  set(_zstd_static_candidates zstd::libzstd_static ZSTD::libzstd_static zstd_static)

  set(_zstd_shared_target "")
  foreach(_candidate IN LISTS _zstd_shared_candidates)
    if(TARGET ${_candidate})
      set(_zstd_shared_target ${_candidate})
      break()
    endif()
  endforeach()

  set(_zstd_static_target "")
  foreach(_candidate IN LISTS _zstd_static_candidates)
    if(TARGET ${_candidate})
      set(_zstd_static_target ${_candidate})
      break()
    endif()
  endforeach()

  if(_zstd_shared_target OR _zstd_static_target)
    if(ZSTD_PREFER_SHARED AND _zstd_shared_target)
      add_library(zstd::zstd ALIAS ${_zstd_shared_target})
      message(STATUS "ZSTD: Using shared library from find_package (${_zstd_shared_target})")
    elseif(_zstd_static_target)
      add_library(zstd::zstd ALIAS ${_zstd_static_target})
      message(STATUS "ZSTD: Using static library from find_package (${_zstd_static_target})")
    elseif(_zstd_shared_target)
      add_library(zstd::zstd ALIAS ${_zstd_shared_target})
      message(STATUS "ZSTD: Falling back to shared library from find_package (${_zstd_shared_target})")
    endif()
    return()
  endif()

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

  set(ZSTD_FOUND TRUE PARENT_SCOPE)

  # define a helper to build both static and shared variants
  macro(build_zstd_variant TYPE SUFFIX)
    set(target libzstd_${SUFFIX})
    add_library(${target} ${TYPE} ${Sources} ${Headers})
    set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)

    add_library(zstd::${target} INTERFACE IMPORTED)
    set_target_properties(zstd::${target} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${zstd_SOURCE_DIR}/lib
      INTERFACE_LINK_LIBRARIES ${target})
  endmacro()

  # now build both
  build_zstd_variant(STATIC static)

  # Create the preferred alias
  add_library(zstd::zstd ALIAS zstd::libzstd_static)
  message(STATUS "ZSTD: Built static library from source")

endfunction()
