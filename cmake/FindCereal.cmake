if (Cereal_LIBRARIES AND Cereal_INCLUDE_DIRS)
  # in cache already
  set(Cereal_FOUND TRUE)
else (Cereal_LIBRARIES AND Cereal_INCLUDE_DIRS)

  # build cereal before we search for required Cereal plugin libs below
  execute_process(
    COMMAND scons -j8 --minimal -u
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/openpilot/cereal
  )

  execute_process(
    COMMAND scons -j8 --minimal -u
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/openpilot/msgq
  )

  find_path(Cereal_INCLUDE_DIRS
    NAMES
      bzlib.h
      capnp
      zmq
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )

  set(Cereal_INCLUDE_DIRS
    ${Cereal_INCLUDE_DIRS}
  )

  find_library(Cereal_LIBRARY_CEREAL
    NAMES
      cereal
    PATHS
      ${CMAKE_SOURCE_DIR}/3rdparty/openpilot/cereal
  )

  find_library(Cereal_LIBRARY_MSGQ
    NAMES
      msgq
    PATHS
      ${CMAKE_SOURCE_DIR}/3rdparty/openpilot/msgq_repo
  )

  find_library(Cereal_LIBRARY_ZMQ
    NAMES
      zmq
  )

  set(Cereal_LIBRARIES
      ${Cereal_LIBRARIES}
      ${Cereal_LIBRARY_CEREAL}
      ${Cereal_LIBRARY_MSGQ}
      ${Cereal_LIBRARY_ZMQ}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Cereal DEFAULT_MSG Cereal_LIBRARIES Cereal_INCLUDE_DIRS)

  mark_as_advanced(Cereal_INCLUDE_DIRS Cereal_LIBRARIES)

endif(Cereal_LIBRARIES AND Cereal_INCLUDE_DIRS)

message("Cereal include dirs: ${Cereal_INCLUDE_DIRS}")
message("Cereal libs: ${Cereal_LIBRARIES}")
