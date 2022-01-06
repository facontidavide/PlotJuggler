
include_directories( ../  )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)


QT5_WRAP_UI ( UI_SRC
    lua_editor.ui
    )

add_library(ToolboxLuaEditor SHARED
    lua_editor.cpp
    lua_editor.h
    ${UI_SRC}  )

target_link_libraries(ToolboxLuaEditor
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    plotjuggler_base
    plotjuggler_qwt
    lua_static
    )

install(TARGETS ToolboxLuaEditor DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
