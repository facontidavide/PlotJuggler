include_directories(../)

qt5_wrap_ui(UI_SRC toolbox_csv.ui)

add_library(ToolboxCSV SHARED
  toolbox_csv.cpp
  RangeSlider.cpp
  ${UI_SRC}
)

find_package(Arrow CONFIG QUIET)
find_package(Parquet CONFIG QUIET)

target_link_libraries(
  ToolboxCSV PRIVATE
  Qt5::Widgets
  Qt5::Xml
  plotjuggler_base
  plotjuggler_qwt
)

if(Arrow_FOUND AND Parquet_FOUND)
  target_link_libraries(ToolboxCSV PRIVATE Arrow::arrow_shared Parquet::parquet_shared)
  target_compile_definitions(ToolboxCSV PRIVATE TOOLBOXCSV_WITH_PARQUET=1)
else()
  target_compile_definitions(ToolboxCSV PRIVATE TOOLBOXCSV_WITH_PARQUET=0)
endif()

target_compile_definitions(ToolboxCSV PRIVATE QT_PLUGIN)

install(TARGETS ToolboxCSV DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
