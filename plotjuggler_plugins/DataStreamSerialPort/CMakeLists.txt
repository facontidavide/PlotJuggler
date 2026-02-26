find_package(Qt5 REQUIRED COMPONENTS SerialPort)

qt5_wrap_ui(UI_SRC datastream_serialport.ui)

set(SRC datastream_serialport.h datastream_serialport.cpp)

add_library(DataStreamSerialPort SHARED ${SRC} ${UI_SRC})

target_link_libraries(
DataStreamSerialPort PRIVATE Qt5::Widgets Qt5::SerialPort plotjuggler_base)

target_compile_definitions(DataStreamSerialPort PRIVATE QT_PLUGIN)

install(TARGETS DataStreamSerialPort
      DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
