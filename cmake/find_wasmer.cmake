
function(find_wasmer)

  find_path(WASMER_INCLUDE_DIR wasmer.h
      HINTS
        ENV WASMER_DIR
        "$ENV{HOME}/.wasmer"
      PATH_SUFFIXES include)

  find_library(WASMER_LIBRARY
      NAMES wasmer
      HINTS
        ENV WASMER_DIR
        "$ENV{HOME}/.wasmer"
      PATH_SUFFIXES lib)

  if(WASMER_INCLUDE_DIR AND WASMER_LIBRARY)
    message(STATUS "Found wasmer: ${WASMER_LIBRARY} (include: ${WASMER_INCLUDE_DIR})")

    add_library(wasmer::wasmer INTERFACE IMPORTED)
    set_target_properties(wasmer::wasmer PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${WASMER_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${WASMER_LIBRARY};pthread;dl;m")

    set(wasmer_FOUND TRUE PARENT_SCOPE)
  else()
    message(STATUS "wasmer not found (set WASMER_DIR or install to ~/.wasmer)")
    set(wasmer_FOUND FALSE PARENT_SCOPE)
  endif()

endfunction()
