function(find_or_download_vector_blf)

  if(TARGET vector_blf::vector_blf)
    message(STATUS "vector_blf target already defined")
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  find_package(vector_blf QUIET CONFIG)
  find_package(Vector_BLF QUIET CONFIG)

  if(TARGET vector_blf::vector_blf)
    message(STATUS "Found vector_blf in system")
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  if(TARGET Vector_BLF::Vector_BLF)
    message(STATUS "Found Vector_BLF in system")
    add_library(vector_blf::vector_blf INTERFACE IMPORTED)
    set_target_properties(
      vector_blf::vector_blf
      PROPERTIES INTERFACE_LINK_LIBRARIES Vector_BLF::Vector_BLF)
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  find_library(VECTOR_BLF_LIBRARY NAMES Vector_BLF)
  find_path(VECTOR_BLF_INCLUDE_DIR NAMES Vector/BLF/File.h)
  if(VECTOR_BLF_LIBRARY AND VECTOR_BLF_INCLUDE_DIR)
    message(STATUS "Found Vector_BLF via library/header lookup")
    add_library(vector_blf::vector_blf INTERFACE IMPORTED)
    set_target_properties(
      vector_blf::vector_blf
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${VECTOR_BLF_INCLUDE_DIR}"
                 INTERFACE_LINK_LIBRARIES "${VECTOR_BLF_LIBRARY}")
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  message(STATUS "vector_blf not found, downloading")
  cpmaddpackage(
    NAME vector_blf
    URL https://github.com/Technica-Engineering/vector_blf/archive/refs/tags/v2.4.2.zip
    OPTIONS "OPTION_RUN_DOXYGEN OFF"
            "OPTION_BUILD_EXAMPLES OFF"
            "OPTION_BUILD_TESTS OFF")

  if(TARGET vector_blf::vector_blf)
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
  elseif(TARGET Vector_BLF::Vector_BLF)
    add_library(vector_blf::vector_blf INTERFACE IMPORTED)
    set_target_properties(
      vector_blf::vector_blf
      PROPERTIES INTERFACE_LINK_LIBRARIES Vector_BLF::Vector_BLF)
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
  elseif(TARGET Vector_BLF)
    add_library(vector_blf::vector_blf INTERFACE IMPORTED)
    set_target_properties(vector_blf::vector_blf PROPERTIES INTERFACE_LINK_LIBRARIES
                                                            Vector_BLF)
    set(VECTOR_BLF_FOUND TRUE PARENT_SCOPE)
  else()
    message(WARNING "vector_blf download completed but no known CMake target was found")
  endif()

endfunction()
