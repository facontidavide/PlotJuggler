
if(BUILDING_WITH_CONAN)

    find_package(lz4 CONFIG REQUIRED)
    find_package(zstd CONFIG REQUIRED)

    set(MCAP_DEPENDENCIES
        lz4::lz4
        zstd::libzstd_static)

    set(MCAP_DEPENDENCIES_FOUND ON)
else()

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
    find_package(LZ4 REQUIRED)
    find_package(Zstd REQUIRED)

    if(LZ4_FOUND AND Zstd_FOUND)
        set(MCAP_DEPENDENCIES_FOUND ON)

        set(MCAP_DEPENDENCIES
            ${Zstd_LIBRARIES}
            ${LZ4_LIBRARIES})
    endif()
endif()
###########################################

if(MCAP_DEPENDENCIES_FOUND)

    SET(INTROSPECTION_TESTS OFF CACHE BOOL "Compile the tests")
    add_subdirectory(rosx_introspection)

    include_directories(  ../  rosx_introspection/include )

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
        ${MCAP_DEPENDENCIES}
        rosx_introspection
        plotjuggler_base )

    install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

else()
    message("MCAP dependencies not found")
endif()
