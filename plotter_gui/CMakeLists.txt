
include_directories( ./ ../  ../include  ../common)

QT5_ADD_RESOURCES (RES_SRC  resource.qrc ../doc/doc_resources.qrc)

QT5_WRAP_UI ( UI_SRC
    aboutdialog.ui
    mainwindow.ui
    removecurvedialog.ui
    busydialog.ui
    curvecolorpick.ui
    filterablelistwidget.ui
    tabbedplotwidget.ui
    help_dialog.ui
    ../common/selectlistdialog.ui
    )

SET( PLOTTER_SRC
    aboutdialog.cpp
    main.cpp
    mainwindow.cpp
    plotwidget.cpp
    plotmatrix.cpp
    removecurvedialog.cpp
    plotmagnifier.cpp
    plotzoomer.cpp
    busydialog.cpp
    busytaskdialog.cpp
    customtracker.cpp
    curvecolorpick.cpp
    timeseries_qwt.cpp
    filterablelistwidget.cpp
    tabbedplotwidget.cpp
    subwindow.cpp

    mainwindow.h
    ../common/selectlistdialog.h
    ../include/PlotJuggler/plotdata.h
    )

add_executable(PlotJuggler ${PLOTTER_SRC} ${RES_SRC} ${UI_SRC})

if( Qt5Svg_FOUND)
    target_link_libraries(PlotJuggler
        Qt5::Core
        Qt5::Widgets
        Qt5::PrintSupport
        Qt5::Xml
        Qt5::Concurrent
        Qt5::Svg
        colorwidgets
        qwt_static )
else()
    target_link_libraries(PlotJuggler
        Qt5::Core
        Qt5::Widgets
        Qt5::PrintSupport
        Qt5::Xml
        Qt5::Concurrent
        colorwidgets
        qwt_static )
endif()


if(COMPILING_WITH_CATKIN)
    install(TARGETS PlotJuggler
       ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
     )
else()
    install(TARGETS PlotJuggler DESTINATION bin  )
endif()
