# - Try to find ZeroMQ
# Once done this will define
# ZeroMQ_FOUND - System has ZeroMQ
# ZeroMQ_INCLUDE_DIRS - The ZeroMQ include directories
# ZeroMQ_LIBRARIES - The libraries needed to use ZeroMQ
# ZeroMQ_DEFINITIONS - Compiler switches required for using ZeroMQ

find_path ( ZeroMQ_INCLUDE_DIR zmq.h )
find_library ( ZeroMQ_LIBRARY NAMES zmq )

set ( ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY} )
set ( ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR} )

include ( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set ZeroMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args ( ZeroMQ DEFAULT_MSG ZeroMQ_LIBRARY ZeroMQ_INCLUDE_DIR )