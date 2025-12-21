


qt5_wrap_ui(UI_SRC dialog_mcap.ui)

add_library(DataLoadMCAP SHARED dataload_mcap.cpp dialog_mcap.cpp ${UI_SRC})
target_include_directories(DataLoadMCAP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)

target_link_libraries(DataLoadMCAP
   PRIVATE
    LZ4::lz4_static
    zstd::libzstd_static)

target_link_libraries(DataLoadMCAP
   PRIVATE
    Qt5::Widgets
    Qt5::Xml
    Qt5::Concurrent
    plotjuggler_base)

# Suppress LNK4217 warnings on Windows for MCAP static library symbols
if(WIN32 AND MSVC)
  target_link_options(DataLoadMCAP PRIVATE /ignore:4217)
endif()

install(TARGETS DataLoadMCAP DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
