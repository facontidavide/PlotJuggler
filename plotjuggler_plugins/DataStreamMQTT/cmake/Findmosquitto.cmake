# - Find libmosquitto
# Find the native libmosquitto includes and libraries
#
#  mosquitto_INCLUDE_DIR - where to find mosquitto.h, etc.
#  mosquitto_LIBRARIES   - List of libraries when using libmosquitto.
#  mosquitto_FOUND       - True if libmosquitto found.

if (NOT mosquitto_INCLUDE_DIR)
  find_path(mosquitto_INCLUDE_DIR mosquitto.h)
endif()

if (NOT mosquitto_LIBRARY)
  find_library(
    mosquitto_LIBRARY
    NAMES mosquitto)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  MOSQUITTO DEFAULT_MSG
  mosquitto_LIBRARY mosquitto_INCLUDE_DIR)

message(STATUS "libmosquitto include dir: ${MOSQUITTO_INCLUDE_DIR}")
message(STATUS "libmosquitto: ${MOSQUITTO_LIBRARY}")
set(mosquitto_LIBRARIES ${mosquitto_LIBRARY})

mark_as_advanced(mosquitto_INCLUDE_DIR mosquitto_LIBRARIES)