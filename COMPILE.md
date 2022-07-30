# Compile in Windows

Dependencies in Windows are managed either using conan or vcpkg

The rest of this section assumes that you installed
You need to install first [Qt](https://www.qt.io/download-open-source) and 
[git](https://desktop.github.com/).

**Visual studio 2019 (16)**, that is part of the Qt 5.15.x installation,
 will be used to compile PlotJuggler.

Start creating a folder called **plotjuggler_ws** and cloning the repo:

```
cd \
mkdir plotjuggler_ws
cd plotjuggler_ws
git clone https://github.com/facontidavide/PlotJuggler.git src/PlotJuggler
```

### Build with Conan

```
conan install src/PlotJuggler --install-folder build/PlotJuggler ^
      --build=missing -pr:b=default

set CMAKE_TOOLCHAIN=%cd%/build/PlotJuggler/conan_toolchain.cmake

cmake -G "Visual Studio 16" ^
      -S PlotJuggler -B build/PlotJuggler ^
      -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN%  ^
      -DCMAKE_INSTALL_PREFIX=%cd%install ^
      -DCMAKE_POLICY_DEFAULT_CMP0091=NEW

cmake --build build/PlotJuggler --config Release --parallel --target install
```

### Build with vcpkg

Change the path where **vcpkg.cmake** can be found as needed.

```
set CMAKE_TOOLCHAIN=/path/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake -G "Visual Studio 16" ^
      -S src/PlotJuggler -B build/PlotJuggler ^
      -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN%  ^
      -DCMAKE_INSTALL_PREFIX=%cd%install

cmake --build build/PlotJuggler --config Release --parallel --target install
```

