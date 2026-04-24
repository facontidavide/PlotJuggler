function(find_or_download_data_tamer)

  if(TARGET data_tamer_parser OR TARGET data_tamer::parser)
    return()
  endif()

  # PlotJuggler only consumes the header-only data_tamer parser. We deliberately
  # avoid find_package(data_tamer_cpp) because its ament CMake config pulls in
  # rclcpp transitively (builtin_interfaces -> rosidl_typesupport_c), which
  # breaks configure on ROS rolling where get_used_typesupports() errors out
  # when called from a non-IDL package. Instead, locate the installed header
  # directly -- on ROS 2 buildfarms data_tamer_cpp is rosdep-installed and its
  # headers land under <prefix>/include/data_tamer_parser/.
  find_path(DATA_TAMER_PARSER_INCLUDE_DIR
    NAMES data_tamer_parser/data_tamer_parser.hpp
  )

  if(DATA_TAMER_PARSER_INCLUDE_DIR)
    message(STATUS "Found data_tamer parser headers: ${DATA_TAMER_PARSER_INCLUDE_DIR}")
    add_library(data_tamer_parser INTERFACE)
    target_include_directories(data_tamer_parser
      INTERFACE "${DATA_TAMER_PARSER_INCLUDE_DIR}")
    add_library(data_tamer::parser ALIAS data_tamer_parser)
    return()
  endif()

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

endfunction()
