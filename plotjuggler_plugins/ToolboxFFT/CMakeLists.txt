include_directories(../)

add_subdirectory(3rdparty/KissFFT)

qt5_wrap_ui(UI_SRC toolbox_FFT.ui)

add_library(ToolboxFFT SHARED toolbox_FFT.cpp toolbox_FFT.h ${UI_SRC})

target_include_directories(ToolboxFFT PRIVATE 3rdparty)

target_link_libraries(ToolboxFFT PRIVATE Qt5::Widgets Qt5::Xml kissfft
                                         plotjuggler_base plotjuggler_qwt)

target_compile_definitions(ToolboxFFT PRIVATE QT_PLUGIN)

install(TARGETS ToolboxFFT DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
