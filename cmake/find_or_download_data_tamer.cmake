function(find_or_download_data_tamer)

  find_package(data_tamer_cpp QUIET)

  if(data_tamer_cpp_FOUND)
    message(STATUS "Found data_tamer in system")
    add_library(data_tamer::parser ALIAS ${data_tamer_cpp_TARGETS})

  elseif(NOT TARGET data_tamer_parser AND NOT TARGET data_tamer::parser)

    message(STATUS "data_tamer not found, downloading")
    cpmaddpackage(
      NAME data_tamer URL
      https://github.com/PickNikRobotics/data_tamer/archive/refs/tags/1.0.3.zip
      DOWNLOAD_ONLY YES)

    add_library(data_tamer_parser INTERFACE)
    target_include_directories(
      data_tamer_parser
      INTERFACE "${data_tamer_SOURCE_DIR}/data_tamer_cpp/include")
    add_library(data_tamer::parser ALIAS data_tamer_parser)

  endif()

endfunction()
