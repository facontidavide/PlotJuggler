# Compile in Linux

On Ubuntu (20.04/22.04), the dependencies can be installed with the command:

```shell
sudo apt -y install qtbase5-dev libqt5svg5-dev libqt5websockets5-dev \
      libqt5opengl5-dev libqt5x11extras5-dev libprotoc-dev libzmq3-dev \
      liblz4-dev libzstd-dev
```

On Fedora:

```shell
sudo dnf install qt5-qtbase-devel qt5-qtsvg-devel qt5-websockets-devel \
      qt5-qtopendl-devel qt5-qtx11extras-devel
```

Clone the repository into **~/plotjuggler_ws**:

```shell
git clone https://github.com/facontidavide/PlotJuggler.git ~/plotjuggler_ws/src/PlotJuggler
cd ~/plotjuggler_ws
```

Then compile using cmake (qmake is NOT supported):

```shell
cmake -S src/PlotJuggler -B build/PlotJuggler -DCMAKE_INSTALL_PREFIX=install
cmake --build build/PlotJuggler --config RelWithDebInfo --target install
```

## Optional: build with Conan

If you want to use [conan](https://conan.io/) to manage the dependencies,
follow this instructions instead.

```shell
conan install src/PlotJuggler --install-folder build/PlotJuggler \
      --build missing -pr:b=default

export CMAKE_TOOLCHAIN=$(pwd)/build/PlotJuggler/conan_toolchain.cmake

cmake -S src/PlotJuggler -B build/PlotJuggler \
      -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN  \
      -DCMAKE_INSTALL_PREFIX=install \
      -DCMAKE_POLICY_DEFAULT_CMP0091=NEW

cmake --build build/PlotJuggler --config RelWithDebInfo --target install
```

## Deploy as an AppImage

Compile and install as described earlier.

Download (once) linuxdeploy:

```shell
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

chmod +x linuxdeploy*.AppImage
mkdir -p AppDir/usr/bin
```

Then:

```shell
cd src/PlotJuggler;export VERSION=$(git describe --abbrev=0 --tags);cd -
echo $VERSION
cp -v install/bin/* AppDir/usr/bin

./linuxdeploy-x86_64.AppImage --appdir=AppDir \
    -d ./src/PlotJuggler/PlotJuggler.desktop \
    -i ./src/PlotJuggler/plotjuggler.png \
    --plugin qt --output appimage
```

## Deploy as an AppImage via Docker

```shell
docker buildx build -o . .
```

# Compile in Mac

On Mac, the dependencies can be installed using [brew](https://brew.sh/) with the following command:

```shell
brew install cmake qt@5 protobuf mosquitto zeromq zstd zmqpp
```

If a newer version of qt is installed, you may need to temporarily link to qt5

```shell
brew link qt@5 --override
# brew link qt --override # Run once you are done building to restore the original linking
```

Add CMake into your env-vars to be detected by cmake

```shell
echo  'QT_HOME=$(brew --prefix qt@5) \
export CPPFLAGS="-I $QT_HOME/include" \
export PKG_CONFIG_PATH="$QT_HOME/lib/pkgconfig" \
```

If you don't want to permanently add them into your main file, you can try by just exporting locally in the current terminal with:

```shell
QT_HOME=$(brew --prefix qt@5)
export CPPFLAGS="-I $QT_HOME/include"
export PKG_CONFIG_PATH="$QT_HOME/lib/pkgconfig"
```

Clone the repository into **~/plotjuggler_ws**:

```shell
git clone https://github.com/facontidavide/PlotJuggler.git ~/plotjuggler_ws/src/PlotJuggler
cd ~/plotjuggler_ws
```

Then compile using cmake:

```shell
cmake -S src/PlotJuggler -B build/PlotJuggler -DCMAKE_INSTALL_PREFIX=install
cmake --build build/PlotJuggler --config RelWithDebInfo --target install
```

# Compile in Windows

Dependencies in Windows are managed either using
[conan](https://conan.io/) or [vcpkg](https://vcpkg.io/en/index.html)

The rest of this section assumes that you installed
You need to install first [Qt](https://www.qt.io/download-open-source) and
[git](https://desktop.github.com/).

**Visual studio 2019 (16)**, that is part of the Qt 5.15.x installation,
 will be used to compile PlotJuggler.

Start creating a folder called **plotjuggler_ws** and cloning the repo:

```batch
cd \
mkdir plotjuggler_ws
cd plotjuggler_ws
git clone https://github.com/facontidavide/PlotJuggler.git src/PlotJuggler
```

## Build with Conan

Note: the Arrow/Parque plugin is not supported in Conan. Use vcpkg instead, if you need
that specific plugin.

```batch
conan install src/PlotJuggler --install-folder build/PlotJuggler ^
      --build=missing -pr:b=default

set CMAKE_TOOLCHAIN=%cd%/build/PlotJuggler/conan_toolchain.cmake

cmake -G "Visual Studio 16" ^
      -S src/PlotJuggler -B build/PlotJuggler ^
      -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN%  ^
      -DCMAKE_INSTALL_PREFIX=%cd%/install ^
      -DCMAKE_POLICY_DEFAULT_CMP0091=NEW


cmake --build build/PlotJuggler --config Release --target install
```

## Build with vcpkg

Change the path where **vcpkg.cmake** can be found as needed.

```batch
set CMAKE_TOOLCHAIN=/path/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake -G "Visual Studio 16" ^
      -S src/PlotJuggler -B build/PlotJuggler ^
      -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN%  ^
      -DCMAKE_INSTALL_PREFIX=%cd%/install

cmake --build build/PlotJuggler --config Release --target install
```

## Create a Windows installer

Change the **Qt** and **QtInstallerFramework** version as needed.

```batch
xcopy src\PlotJuggler\installer installer\ /Y /S /f /z
xcopy install\bin\*.* installer\io.plotjuggler.application\data /Y /S /f /z

C:\QtPro\5.15.16\msvc2019_64\bin\windeployqt.exe --release installer\io.plotjuggler.application\data\plotjuggler.exe

C:\QtPro\Tools\QtInstallerFramework\4.6\bin\binarycreator.exe --offline-only -c installer\config.xml -p installer  PlotJuggler-Windows-installer.exe
```
