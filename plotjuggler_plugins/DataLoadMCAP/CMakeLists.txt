add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )

add_library(DataLoadMCAP SHARED
    mcap.cpp
    dataload_mcap.cpp
    dialog_mcap.cpp
    ${UI_SRC}  )

target_link_libraries(DataLoadMCAP
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    libzstd_static
    lz4_static
    plotjuggler_base )

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
