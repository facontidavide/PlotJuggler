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

  message(STATUS "vector_blf not found, downloading")
  cpmaddpackage(
    NAME vector_blf
    URL https://github.com/Technica-Engineering/vector_blf/archive/refs/tags/v2.4.2.zip)

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
