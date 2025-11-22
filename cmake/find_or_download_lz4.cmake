function(find_or_download_lz4)

  if(TARGET LZ4::lz4)
    message(STATUS "LZ4 targets already defined")
    return()
  endif()

  find_package(LZ4 QUIET)

  # Determine which library type to use
  if(PREFER_DYNAMIC_LZ4)
    set(LZ4_PREFER_SHARED ON)
    message(STATUS "LZ4: Preferring dynamic library")
  else()
    set(LZ4_PREFER_SHARED OFF)
    message(STATUS "LZ4: Preferring static library")
  endif()

  # Reuse any existing LZ4 targets from find_package or toolchains
  set(_lz4_base_candidates LZ4::lz4 lz4::lz4 lz4)
  foreach(_candidate IN LISTS _lz4_base_candidates)
    if(TARGET ${_candidate})
      add_library(LZ4::lz4 ALIAS ${_candidate})
      message(STATUS "LZ4: Using existing target ${_candidate}")
      return()
    endif()
  endforeach()

  set(_lz4_shared_candidates LZ4::lz4_shared lz4::lz4_shared lz4_shared)
  set(_lz4_static_candidates LZ4::lz4_static lz4::lz4_static lz4_static)

  set(_lz4_shared_target "")
  foreach(_candidate IN LISTS _lz4_shared_candidates)
    if(TARGET ${_candidate})
      set(_lz4_shared_target ${_candidate})
      break()
    endif()
  endforeach()

  set(_lz4_static_target "")
  foreach(_candidate IN LISTS _lz4_static_candidates)
    if(TARGET ${_candidate})
      set(_lz4_static_target ${_candidate})
      break()
    endif()
  endforeach()

  if(_lz4_shared_target OR _lz4_static_target)
    if(LZ4_PREFER_SHARED AND _lz4_shared_target)
      add_library(LZ4::lz4 ALIAS ${_lz4_shared_target})
      message(STATUS "LZ4: Using shared library from find_package (${_lz4_shared_target})")
    elseif(_lz4_static_target)
      add_library(LZ4::lz4 ALIAS ${_lz4_static_target})
      message(STATUS "LZ4: Using static library from find_package (${_lz4_static_target})")
    elseif(_lz4_shared_target)
      add_library(LZ4::lz4 ALIAS ${_lz4_shared_target})
      message(STATUS "LZ4: Falling back to shared library from find_package (${_lz4_shared_target})")
    endif()
    return()
  endif()

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

  # Create the preferred alias
  add_library(LZ4::lz4 ALIAS LZ4::lz4_static)
  message(STATUS "LZ4: Built static library from source")

endfunction()
