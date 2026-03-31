function(find_or_download_dbcppp)
  if(DEFINED PJ_ENABLE_DBCPPP AND NOT PJ_ENABLE_DBCPPP)
    message(STATUS "dbcppp support disabled (PJ_ENABLE_DBCPPP=OFF)")
    set(DBCPPP_FOUND FALSE PARENT_SCOPE)
    return()
  endif()

  if(TARGET dbcppp::dbcppp)
    message(STATUS "dbcppp target already defined")
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  find_package(dbcppp QUIET CONFIG)

  if(TARGET dbcppp::dbcppp)
    message(STATUS "Found dbcppp in system")
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  if(TARGET dbcppp)
    get_target_property(_dbcppp_target_type dbcppp TYPE)
    if(NOT _dbcppp_target_type STREQUAL "EXECUTABLE")
      message(STATUS "Found dbcppp in system")
      add_library(dbcppp::dbcppp INTERFACE IMPORTED)
      set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES dbcppp)
      set(DBCPPP_FOUND TRUE PARENT_SCOPE)
      return()
    endif()
  endif()

  if(TARGET libdbcppp)
    message(STATUS "Found libdbcppp in system")
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES libdbcppp)
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  find_library(DBCPPP_LIBRARY NAMES dbcppp)
  find_path(DBCPPP_INCLUDE_DIR NAMES dbcppp/Network.h)
  if(DBCPPP_LIBRARY AND DBCPPP_INCLUDE_DIR)
    message(STATUS "Found dbcppp via library/header lookup")
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${DBCPPP_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "${DBCPPP_LIBRARY}")
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
    return()
  endif()

  message(STATUS "dbcppp not found, downloading")
  set(_dbcppp_cpm_args
    NAME dbcppp
    GIT_REPOSITORY https://github.com/xR3b0rn/dbcppp.git
    GIT_TAG 54ce78ad53442db94e03d16a4fd5f3b339dab193
    GIT_SHALLOW TRUE
    GIT_SUBMODULES
      third-party/cxxopts
      third-party/libxml2
      third-party/libxmlmm
    OPTIONS "build_tests OFF"
            "build_examples OFF"
            "LIBXML2_WITH_PROGRAMS OFF")

  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
    list(APPEND _dbcppp_cpm_args GIT_SUBMODULES_RECURSE TRUE)
  else()
    message(STATUS "CMake < 3.17 detected, skipping GIT_SUBMODULES_RECURSE for dbcppp")
  endif()

  cpmaddpackage(${_dbcppp_cpm_args})

  if(TARGET dbcppp::dbcppp)
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
  elseif(TARGET libdbcppp)
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES libdbcppp)
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
  elseif(TARGET dbcppp)
    get_target_property(_dbcppp_target_type dbcppp TYPE)
    if(NOT _dbcppp_target_type STREQUAL "EXECUTABLE")
      add_library(dbcppp::dbcppp INTERFACE IMPORTED)
      set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES dbcppp)
      set(DBCPPP_FOUND TRUE PARENT_SCOPE)
    else()
      message(WARNING "dbcppp target exists but is an executable; libdbcppp target not found")
    endif()
  else()
    message(WARNING "dbcppp download completed but no known CMake target was found")
  endif()

endfunction()
