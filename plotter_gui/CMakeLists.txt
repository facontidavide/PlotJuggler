
include_directories( ./ ../  ../include  ../common)

QT5_ADD_RESOURCES (RES_SRC  resource.qrc )

QT5_WRAP_UI ( UI_SRC
    aboutdialog.ui
    mainwindow.ui
    removecurvedialog.ui
    busydialog.ui
    curvecolorpick.ui
    filterablelistwidget.ui
    tabbedplotwidget.ui
    axis_limits_dialog.ui
    add_math_plot.ui
    ../common/selectlistdialog.ui
    )

SET( PLOTTER_SRC
    aboutdialog.cpp
    axis_limits_dialog.cpp
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
    add_math_plot.cpp
    math_plot.cpp
    tree_completer.h

    mainwindow.h
    realslider.h
    utils.h
    ../common/selectlistdialog.h
    ../include/PlotJuggler/plotdata.h
    ../include/PlotJuggler/datastreamer_base.h
    )

add_executable(PlotJuggler ${PLOTTER_SRC} ${RES_SRC} ${UI_SRC} ${BACKWARD_SRC})

target_link_libraries(PlotJuggler
    ${QT_LINK_LIBRARIES}
    colorwidgets
    qwt_static
    ${BACKWARD_LIBS}
    )

if(COMPILING_WITH_CATKIN)
    install(TARGETS PlotJuggler
       ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
     )
else()
    install(TARGETS PlotJuggler DESTINATION bin  )
endif()
