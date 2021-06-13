
set (CMAKE_CXX_STANDARD 17)
set( CMAKE_CXX_STANDARD_REQUIRED ON )

#########################  LIBRARY  ####################################

qt5_wrap_cpp(PLOTJUGGLER_BASE_MOCS
  include/PlotJuggler/dataloader_base.h
  include/PlotJuggler/datastreamer_base.h
  include/PlotJuggler/statepublisher_base.h
  include/PlotJuggler/transform_function.h
)

add_library( plotjuggler_plugin_base STATIC ${PLOTJUGGLER_BASE_MOCS})
target_link_libraries( plotjuggler_plugin_base PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${catkin_LIBRARIES} )

if(COMPILING_WITH_CATKIN)

elseif(COMPILING_WITH_AMENT)
    ament_export_targets(export_plotjuggler_plugin_base HAS_LIBRARY_TARGET)
else()

endif()

install(
    TARGETS plotjuggler_plugin_base
    EXPORT export_plotjuggler_plugin_base
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
    )

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include )
