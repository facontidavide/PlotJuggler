include_directories(  ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

SET( SRC
  rlog_parser.cpp
  dataload_rlog.cpp
  )

add_library(DataLoadRlog SHARED ${SRC} ${UI_SRC}  )

target_link_libraries(DataLoadRlog
  ${Qt5Widgets_LIBRARIES}
  ${Qt5Xml_LIBRARIES}
  -lcapnp
  -lcapnpc
  -lbz2
  -lkj
  plotjuggler_plugin_base)

install(TARGETS DataLoadRlog DESTINATION bin  )
