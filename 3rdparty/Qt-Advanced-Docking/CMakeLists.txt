cmake_minimum_required(VERSION 3.5)

project(QtADS LANGUAGES CXX)

include_directories(include/Qads)

find_package(Qt5 5.5 COMPONENTS Core Gui Widgets REQUIRED)
if (UNIX AND NOT APPLE)
        find_package(Qt5 5.5 COMPONENTS X11Extras REQUIRED)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(ads_SRCS
    src/ads_globals.cpp
    src/DockAreaTabBar.cpp
    src/DockAreaTitleBar.cpp
    src/DockAreaWidget.cpp
    src/DockContainerWidget.cpp
    src/DockManager.cpp
    src/DockOverlay.cpp
    src/DockSplitter.cpp
    src/DockWidget.cpp
    src/DockWidgetTab.cpp
    src/DockingStateReader.cpp
    src/DockFocusController.cpp
    src/ElidingLabel.cpp
    src/FloatingDockContainer.cpp
    src/FloatingDragPreview.cpp
    src/IconProvider.cpp
    src/DockComponentsFactory.cpp

    src/ads.qrc

    include/Qads/ads_globals.h
    include/Qads/DockAreaTabBar.h
    include/Qads/DockAreaTitleBar.h
    include/Qads/DockAreaTitleBar_p.h
    include/Qads/DockAreaWidget.h
    include/Qads/DockContainerWidget.h
    include/Qads/DockManager.h
    include/Qads/DockOverlay.h
    include/Qads/DockSplitter.h
    include/Qads/DockWidget.h
    include/Qads/DockWidgetTab.h
    include/Qads/DockingStateReader.h
    include/Qads/DockFocusController.h
    include/Qads/ElidingLabel.h
    include/Qads/FloatingDockContainer.h
    include/Qads/FloatingDragPreview.h
    include/Qads/IconProvider.h
    include/Qads/DockComponentsFactory.h
)

if (UNIX AND NOT APPLE)
    set(ads_SRCS
        src/linux/FloatingWidgetTitleBar.cpp
        ${ads_SRCS}
    )
endif()


add_library(qt_advanced_docking STATIC ${ads_SRCS} )
target_compile_definitions(qt_advanced_docking PUBLIC ADS_STATIC)

target_link_libraries(qt_advanced_docking PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets)

if(UNIX AND NOT APPLE)
    target_link_libraries(qt_advanced_docking PUBLIC Qt5::X11Extras)
        target_link_libraries(qt_advanced_docking PRIVATE xcb)
endif()

set_target_properties(qt_advanced_docking PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)



