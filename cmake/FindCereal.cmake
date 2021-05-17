if (Cereal_INCLUDE_DIRS)
  # in cache already
  set(Cereal_FOUND TRUE)
  message("Cereal found!")
else (Cereal_LIBRARIES AND Cereal_INCLUDE_DIRS)
  message("cereal not found! searching...")

  find_path(Cereal_INCLUDE_DIRS /home/batman/openpilot/openpilot/cereal/messaging/messaging.h)
  
  set(Cereal_INCLUDE_DIRS
    ${Cereal_INCLUDE_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Cereal DEFAULT_MSG Cereal_INCLUDE_DIRS)

  mark_as_advanced(Cereal_INCLUDE_DIRS)

endif(Cereal_INCLUDE_DIRS)
