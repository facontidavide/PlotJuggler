if (Rlog_INCLUDE_DIRS) 
  # in cache already
  set(Rlog_FOUND TRUE)
else (Rlog_LIBRARIES AND Rlog_INCLUDE_DIRS)
  
  find_path(Rlog_INCLUDE_DIRS 
    NAMES
      bzlib.h
      capnp
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
  )
  
  set(Rlog_INCLUDE_DIRS
    ${Rlog_INCLUDE_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Rlog DEFAULT_MSG Rlog_INCLUDE_DIRS)

  mark_as_advanced(Rlog_INCLUDE_DIRS)

endif(Rlog_INCLUDE_DIRS)
