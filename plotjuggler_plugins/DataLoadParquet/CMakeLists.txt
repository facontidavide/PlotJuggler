find_package(Arrow QUIET)
find_package(Parquet QUIET)

if(Arrow_FOUND)
  message(STATUS "[Parquet] found")

  qt5_wrap_ui(UI_SRC dataload_parquet.ui)

  set(SRC dataload_parquet.cpp)

  add_library(DataLoadParquet SHARED ${SRC} ${UI_SRC})

  target_link_libraries(
    DataLoadParquet PRIVATE Qt5::Widgets Qt5::Xml Arrow::arrow_static
                            Parquet::parquet_static plotjuggler_base)

  target_compile_definitions(DataLoadParquet PRIVATE QT_PLUGIN)

  install(TARGETS DataLoadParquet DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})

else()
  message("[Parquet] not found. Skipping plugin DataLoadParquet.")
endif()
