function(find_or_download_vector_blf)
  if(DEFINED PJ_ENABLE_VECTOR_BLF AND NOT PJ_ENABLE_VECTOR_BLF)
    message(STATUS "vector_blf support disabled (PJ_ENABLE_VECTOR_BLF=OFF)")
    set(VECTOR_BLF_FOUND FALSE PARENT_SCOPE)
    return()
  endif()

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
    GIT_REPOSITORY https://github.com/Technica-Engineering/vector_blf.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
    OPTIONS "OPTION_RUN_DOXYGEN OFF"
            "OPTION_BUILD_EXAMPLES OFF"
            "OPTION_BUILD_TESTS OFF")

  # vector_blf upstream uses CMAKE_SOURCE_DIR in include_directories().
  # When consumed as a subproject, that can point to the parent project and
  # break header lookup on Windows. Force the correct source/build include roots.
  if(TARGET Vector_BLF)
    if(DEFINED vector_blf_SOURCE_DIR)
      target_include_directories(Vector_BLF PRIVATE "${vector_blf_SOURCE_DIR}/src")
      target_include_directories(Vector_BLF PUBLIC "$<BUILD_INTERFACE:${vector_blf_SOURCE_DIR}/src>")
    endif()
    if(DEFINED vector_blf_BINARY_DIR)
      target_include_directories(Vector_BLF PRIVATE "${vector_blf_BINARY_DIR}/src")
      target_include_directories(Vector_BLF PUBLIC "$<BUILD_INTERFACE:${vector_blf_BINARY_DIR}/src>")
    endif()
  endif()

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
