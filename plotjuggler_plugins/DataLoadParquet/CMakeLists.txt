
set(CMAKE_CONFIG_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake" ${CMAKE_MODULE_PATH}  )
set(CMAKE_MODULE_PATH  "${CMAKE_CONFIG_PATH} ${CMAKE_MODULE_PATH}")

find_package(Zstd QUIET)
set (ZSTD_LIB ${Zstd_LIBRARIES})
find_package(Arrow QUIET)
find_package(Parquet QUIET)

if(ARROW_FOUND AND PARQUET_FOUND)

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
        arrow
        parquet
        plotjuggler_base)

    install(TARGETS DataLoadParquet DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("[libparquet-dev] not found. Skipping plugin DataLoadParquet.")
endif()


