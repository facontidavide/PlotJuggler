
include_directories(../)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  publisher_csv_dialog.ui  )

add_library(PublisherCSV SHARED  publisher_csv.cpp  ${UI_SRC}  )

target_link_libraries(PublisherCSV
    ${Qt5Widgets_LIBRARIES}
    plotjuggler_plugin_base
    )


if(COMPILING_WITH_CATKIN)
    install(TARGETS PublisherCSV
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
elseif(COMPILING_WITH_AMENT)
    install(TARGETS PublisherCSV
            DESTINATION  lib/${PROJECT_NAME} )
else()
    install(TARGETS PublisherCSV DESTINATION bin  )
endif()


