# Compilation
These instructions are adjusted to fit our usage of PlotJuggler. For the original compilation instructions, see [here](https://github.com/facontidavide/PlotJuggler/blob/main/COMPILE.md).
## Compile on Ubuntu (tested in 18.04)
### Dependencies

The dependencies can be installed with the command:

    sudo apt -y install qtbase5-dev libqt5svg5-dev libqt5websockets5-dev \
         libqt5opengl5-dev libqt5x11extras5-dev libprotoc-dev libzmq-dev 

Install the latest version (tested till 10.0.1) of Arrow for C++ and Apache Parquet for C++ according to the install instructions [here](https://arrow.apache.org/install/).

### Build the repository

Clone the repository:

```
git clone https://github.com/mtan1503/PlotJuggler.git
cd PlotJuggler
```
    
Then compile using cmake and install to your computer:

```
mkdir build && cd build
cmake ..
make -j8
sudo make install
```
If you run into issues during compilation, check the *Troubleshooting* chapter below.
## Run PlotJuggler
If you installed PlotJuggler, then you can open the app

If you want to run your build, the from your `build/` folder run:
```
./bin/plotjuggler
```
## Troubleshooting
### Compilation issues
#### CMake Error: cannot create imported target
If you get an error similar to the error below, this happens because in an older version of PlotJuggler the CMakeLists used `find_package(Arrow CONFIG)` and `find_package(Parquet CONFIG)`.  
```
CMake Error at /usr/lib/x86_64-linux-gnu/cmake/Arrow/FindBrotli.cmake:122 (add_library):
  add_library cannot create imported target "Brotli::brotlicommon" because another target with the same name already exists.
Call Stack (most recent call first):
  /usr/share/cmake-3.22/Modules/CMakeFindDependencyMacro.cmake:47 (find_package)
  /usr/lib/x86_64-linux-gnu/cmake/Arrow/ArrowConfig.cmake:115 (find_dependency)
  plotjuggler_plugins/DataLoadParquet/cmake/FindArrow.cmake:424 (find_package)
  plotjuggler_plugins/DataLoadParquet/CMakeLists.txt:17 (find_package)
```
To get rid of this issue, delete the Arrow and Parquet folder in the path that specifies where ArrowConfig.cmake is located within this repository. For example, from the main folder of this repository:
```
cd plotjuggler_plugins/DataLoadParquet/
rm -r cmake/
```
### Run time issues

#### Problem: PlotJuggler closes when I open a Parquet file
It is likely that your Parquet/Arrow version and the version of this branch do not align. 

Make sure that you have the latest Arrow version installed (10.0.1).
* Check your current Arrow and Parquet version with `dpkg --list | grep arrow` and `dpkg --list | grep parquet`.
* See instructions above for how to instal the newest version. 

Make sure that you have built this repo with the latest version of Arrow and Parquet.
* See build instructions above.

## Deploy as an AppImage

Compile and install as described earlier.

Download (once) linuxdeploy:

```
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage

wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

chmod +x linuxdeploy*.AppImage
mkdir -p AppDir/usr/bin
```

Then:

```
cd PlotJuggler;export VERSION=$(git describe --abbrev=0 --tags);cd -
echo $VERSION
cp -v install/bin/* AppDir/usr/bin

./linuxdeploy-x86_64.AppImage --appdir=AppDir \
    -d ./PlotJuggler/PlotJuggler.desktop \
    -i ./PlotJuggler/plotjuggler.png \
    --plugin qt --output appimage
```
