#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "plotjuggler::plotjuggler_base" for configuration "Debug"
set_property(TARGET plotjuggler::plotjuggler_base APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(plotjuggler::plotjuggler_base PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libplotjuggler_base.a"
  )

list(APPEND _cmake_import_check_targets plotjuggler::plotjuggler_base )
list(APPEND _cmake_import_check_files_for_plotjuggler::plotjuggler_base "${_IMPORT_PREFIX}/lib/libplotjuggler_base.a" )

# Import target "plotjuggler::plotjuggler_qwt" for configuration "Debug"
set_property(TARGET plotjuggler::plotjuggler_qwt APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(plotjuggler::plotjuggler_qwt PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libplotjuggler_qwt.a"
  )

list(APPEND _cmake_import_check_targets plotjuggler::plotjuggler_qwt )
list(APPEND _cmake_import_check_files_for_plotjuggler::plotjuggler_qwt "${_IMPORT_PREFIX}/lib/libplotjuggler_qwt.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
