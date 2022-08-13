
if(BUILDING_WITH_CONAN)

    find_package(zstd CONFIG REQUIRED)
    set(Zstd_LIBRARIES zstd::libzstd_static)
    set(Zstd_FOUND ON)

else()

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
    find_package(Zstd QUIET)

endif()
###########################################

if(Zstd_FOUND)

    include_directories( ${Zstd_INCLUDE_DIRS} )

    add_definitions(${QT_DEFINITIONS})
    add_definitions(-DQT_PLUGIN)

    QT5_WRAP_UI ( UI_SRC  dialog_mcap.ui  )

    SET( SRC
        mcap.cpp
        dataload_mcap.cpp
        dialog_mcap.cpp
        )

    add_library(DataLoadMCAP SHARED ${SRC} ${UI_SRC}  )
    target_link_libraries(DataLoadMCAP
        ${Qt5Widgets_LIBRARIES}
        ${Qt5Xml_LIBRARIES}
        ${Zstd_LIBRARIES}
        lz4_static
        plotjuggler_base )

    install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("MCAP dependencies (Zstd) not found")
endif()
