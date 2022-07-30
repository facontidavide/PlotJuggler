
find_package(arrow CONFIG QUIET)

if(Arrow_FOUND)
    message(STATUS "[Parquet] found")
    include_directories(  ../
        ${ARROW_INCLUDE_DIR}
        ${PARQUET_INCLUDE_DIR} )

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  dataload_parquet.ui  )

    SET( SRC dataload_parquet.cpp )

    add_library(DataLoadParquet SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataLoadParquet
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        plotjuggler_base)
    target_link_libraries(DataLoadParquet PRIVATE 
       re2::re2 arrow_shared parquet_shared arrow_bundled_dependencies)

    install(TARGETS DataLoadParquet DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("[Parquet] not found. Skipping plugin DataLoadParquet.")
endif()


