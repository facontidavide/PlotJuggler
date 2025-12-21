# * Try to find Zcm Once done this will define Zcm_FOUND - System has Zcm
#   Zcm_INCLUDE_DIRS - The Zcm include directories Zcm_LIBRARIES - The libraries
#   needed to use Zcm Zcm_DEFINITIONS - Compiler switches required for using Zcm

find_path(Zcm_INCLUDE_DIR zcm/zcm-cpp.hpp
          PATHS $ENV{PJ_ZCM_INSTALL_DIR}/include)
find_path(Zcm_INCLUDE_DIR zcm/zcm-cpp.hpp)
find_library(
  Zcm_LIBRARY
  NAMES zcm
  PATHS $ENV{PJ_ZCM_INSTALL_DIR}/lib)
find_library(Zcm_LIBRARY NAMES zcm)
find_library(
  ZcmToolsUtil_LIBRARY
  NAMES zcm_tools_util
  PATHS $ENV{PJ_ZCM_INSTALL_DIR}/lib)
find_library(ZcmToolsUtil_LIBRARY NAMES zcm_tools_util)

set(Zcm_LIBRARIES ${Zcm_LIBRARY} ${ZcmToolsUtil_LIBRARY})
set(Zcm_INCLUDE_DIRS ${Zcm_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set Zcm_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(Zcm DEFAULT_MSG Zcm_LIBRARY Zcm_INCLUDE_DIR)
