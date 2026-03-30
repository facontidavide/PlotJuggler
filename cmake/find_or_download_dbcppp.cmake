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
    message(STATUS "Found dbcppp in system")
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES dbcppp)
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
  cpmaddpackage(
    NAME dbcppp
    GIT_REPOSITORY https://github.com/xR3b0rn/dbcppp.git
    GIT_TAG 54ce78ad53442db94e03d16a4fd5f3b339dab193
    GIT_SHALLOW TRUE
    GIT_SUBMODULES
      third-party/cxxopts
      third-party/libxml2
      third-party/libxmlmm
    GIT_SUBMODULES_RECURSE TRUE
    OPTIONS "build_kcd OFF"
            "build_tools OFF"
            "build_tests OFF"
            "build_examples OFF")

  if(TARGET dbcppp::dbcppp)
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
  elseif(TARGET dbcppp)
    add_library(dbcppp::dbcppp INTERFACE IMPORTED)
    set_target_properties(dbcppp::dbcppp PROPERTIES INTERFACE_LINK_LIBRARIES dbcppp)
    set(DBCPPP_FOUND TRUE PARENT_SCOPE)
  else()
    message(WARNING "dbcppp download completed but no known CMake target was found")
  endif()

endfunction()
