if(BUILDING_WITH_VCPKG)
    message(STATUS "Finding Parquet with vcpkg")
    set(PARQUET_LIBRARIES 
        re2::re2 
        arrow_shared 
        parquet_shared 
        arrow_bundled_dependencies)

    find_package(arrow CONFIG QUIET)

else()
    message(STATUS "Finding Parquet without package managers")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

    find_package(Arrow QUIET)
    if(NOT ${Arrow_FOUND})
        message(FATAL_ERROR "Apache Arrow not found.")
    endif()

    find_package(Parquet QUIET)
    if(NOT ${Parquet_FOUND})
        message(FATAL_ERROR "Parquet not found.")
    endif()

endif()


if(${Arrow_FOUND} AND ${Parquet_FOUND})
    message(STATUS "Arrow version: ${ARROW_VERSION}")
    message(STATUS "Parquet version: ${PARQUET_VERSION}")

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  dataload_parquet.ui  )

    SET( SRC dataload_parquet.cpp )

    add_library(DataLoadParquet SHARED ${SRC} ${UI_SRC}  )

    target_link_libraries(DataLoadParquet 
        Arrow::arrow_shared
        Parquet::parquet_shared 
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        plotjuggler_base)


    install(TARGETS DataLoadParquet DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("Arrow/Parquet not found. Skipping plugin DataLoadParquet.")
endif()
