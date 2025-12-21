include_directories(../)

qt5_wrap_ui(UI_SRC quaternion_to_rpy.ui)

add_library(ToolboxQuaternion SHARED toolbox_quaternion.cpp
                                     quaternion_to_rpy.cpp ${UI_SRC})

target_link_libraries(
  ToolboxQuaternion PRIVATE Qt5::Widgets Qt5::Xml plotjuggler_base
                            plotjuggler_qwt)

target_compile_definitions(ToolboxQuaternion PRIVATE QT_PLUGIN)

install(TARGETS ToolboxQuaternion DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
