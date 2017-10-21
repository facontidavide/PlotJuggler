
include_directories( ./ ../  ../../include  ../../common)

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  ../../common/selectlistdialog.ui  )

SET( SRC
    dataload_csv.cpp
    ../../common/selectlistdialog.h
    )

add_library(DataLoadCSV SHARED ${SRC} ${UI_SRC}  )
target_link_libraries(DataLoadCSV  ${Qt5Widgets_LIBRARIES} ${Qt5Xml_LIBRARIES})


if(COMPILING_WITH_CATKIN)
    install(TARGETS DataLoadCSV
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
    install(TARGETS DataLoadCSV DESTINATION bin  )
endif()
