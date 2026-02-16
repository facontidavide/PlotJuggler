include_directories(../)

qt5_wrap_ui(UI_SRC toolbox_csv.ui)

add_library(ToolboxCSV SHARED
  toolbox_csv.cpp
  RangeSlider.cpp
  ${UI_SRC}
)

target_link_libraries(
  ToolboxCSV PRIVATE
  Qt5::Widgets
  Qt5::Xml
  plotjuggler_base
  plotjuggler_qwt
)

target_compile_definitions(ToolboxCSV PRIVATE QT_PLUGIN)

install(TARGETS ToolboxCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
