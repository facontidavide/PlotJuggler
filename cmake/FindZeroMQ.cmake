# - Try to find ZeroMQ
# Once done this will define
# ZeroMQ_FOUND - System has ZeroMQ
# ZeroMQ_INCLUDE_DIRS - The ZeroMQ include directories
# ZeroMQ_LIBRARIES - The libraries needed to use ZeroMQ
# ZeroMQ_DEFINITIONS - Compiler switches required for using ZeroMQ

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)

set(ZeroMQ_FOUND TRUE)
set(ZeroMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIRS})
set(ZeroMQ_LIBRARIES ${ZMQ_LIBRARIES})
