# Install dependenices with vcpkg

Follow the instructions [here](https://vcpkg.io/en/getting-started.html).
We will assume that both **PlotJuggler** and **vcpkg** are cloned in a directory called `C:\dev`. 


    git clone https://github.com/Microsoft/vcpkg.git
    .\vcpkg\bootstrap-vcpkg.bat

Now, add the folder `C:\dev\vcpkg` to your **PATH**, preferably globally.

If you want to compile with **mingw** set the following variables:

    set VCPKG_DEFAULT_TRIPLET=x64-mingw-dynamic
    set VCPKG_DEFAULT_HOST_TRIPLET=x64-mingw-dynamic

Otherwise, if you want to use the MSVC compiler:

    set VCPKG_DEFAULT_TRIPLET=x64-windows
    set VCPKG_DEFAULT_HOST_TRIPLET=x64-windows

You can now install the dependencies required by PlotJuggler

    vcpkg install protobuf zeromq mosquitto

# Install Qt

Qt5 dependencies in vcpkg are broken at the time of writing.
Download Qt5 from the official web and insatll it in the folder `C:\Qt`.

More informations here: https://github.com/facontidavide/PlotJuggler/discussions/518

# Compile from command line Qt Creator

If you are familiar with Qt Creator, you should follow the usual steps.

Note that `C:\dev\vcpkg` must be in your **PATH** to work correctly.

For more information: https://www.qt.io/blog/qt-creator-cmake-package-manager-auto-setup

# Compile from command line with cmake

Assuming that:

- **PlotJuggler** is in the folder `C:\dev\PlotJuggler`
- **vcpkg**** is in the folder `C:\dev\vcpkg`

Move to folder `C:\dev` and type:

    cmake -B build_pj -S PlotJuggler -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake

And compile in Release mode with:

    cmake --build build_pj --config Release

# Final app deployment

Move to the folder where `plotjuggler.exe` is located (should be `C:\dev\build_pj\bin\release`) and use `windeployqt.exe` to copy the needed
Qt libraries.

For instance, if you have Qt 5.12.2 installed and compiled witg MSVC 2019.

    C:\Qt\5.15.2\msvc2019_64\bin\windeployqt.exe plotjuggler.exe
