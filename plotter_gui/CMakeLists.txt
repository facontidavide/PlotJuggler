
include_directories( ./ ../  ../include  ../common)

QT5_ADD_RESOURCES (RES_SRC  resource.qrc )


QT5_WRAP_UI ( UI_SRC
    aboutdialog.ui
    axis_limits_dialog.ui
    mainwindow.ui
    removecurvedialog.ui
    curvecolorpick.ui
    tabbedplotwidget.ui
    support_dialog.ui
    curvelist_panel.ui
    preferences_dialog.ui
    suggest_dialog.ui

    transforms/function_editor.ui
    transforms/transform_selector.ui

    ../common/selectlistdialog.ui
    )

SET( PLOTTER_SRC
    axis_limits_dialog.cpp
    customtracker.cpp
    curvecolorpick.cpp
    curvelist_panel.cpp
    curvelist_view.cpp
    curvetree_view.cpp
    main.cpp
    mainwindow.cpp
    menubar.cpp
    plotlegend.cpp
    plotwidget.cpp
    plotmatrix.cpp
    plotmagnifier.cpp
    preferences_dialog.cpp
    point_series_xy.cpp
    plotzoomer.cpp
    removecurvedialog.cpp
    subwindow.cpp
    suggest_dialog.cpp
    timeseries_qwt.cpp
    tabbedplotwidget.cpp
    tab_widget.h
    tree_completer.h

    transforms/custom_function.cpp
    transforms/custom_timeseries.cpp
    transforms/function_editor.cpp
    transforms/transform_selector.cpp

    cheatsheet/video_cheatsheet.cpp

    utils.cpp

    mainwindow.h
    realslider.h
    utils.h
    ../common/selectlistdialog.h
    ../include/PlotJuggler/plotdata.h
    ../include/PlotJuggler/datastreamer_base.h
    )

add_executable(PlotJuggler ${PLOTTER_SRC} ${RES_SRC} ${UI_SRC}
    ../3rdparty/backward-cpp/backward.cpp)

target_link_libraries(PlotJuggler
    ${QT_LINK_LIBRARIES}
    colorwidgets
    qwt_static
    ${BACKWARD_LIBS} )

if(COMPILING_WITH_CATKIN)
    install(TARGETS PlotJuggler
       ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
       RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
     )
    install(
        TARGETS PlotJuggler
        RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
    )
else()
    install(TARGETS PlotJuggler DESTINATION bin  )
endif()
