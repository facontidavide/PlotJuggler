# Build the AppImage with catkin_make

Cleanup and build

    rm -rf build devel install AppDir
    catkin_make -DCMAKE_BUILD_TYPE=Release  -j$(nproc) install  
    
    cd src/PlotJuggler;export VERSION=$(git describe --abbrev=0 --tags);cd -;echo $VERSION

## Method 1

In the root folder of ws_plotjuggler:

    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage  ~/linuxdeploy-x86_64.AppImage
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage ~/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x ~/linuxdeploy*.AppImage

    mkdir -p AppDir/usr/bin/
    cp install/lib/plotjuggler/* AppDir/usr/bin
    
    ./linuxdeploy-x86_64.AppImage --appdir=AppDir -d src/PlotJuggler/PlotJuggler.desktop -i src/PlotJuggler/plotjuggler-icon.png  --plugin qt --output appimage
     
## Method 2

    wget "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage" -O ~/linuxdeployq.AppImage
    chmod +x ~/linuxdeployq.AppImage

    cp src/PlotJuggler/PlotJuggler.desktop ./install/lib/plotjuggler/
    cp src/PlotJuggler/plotjuggler.png     ./install/lib/plotjuggler/

    ~/linuxdeployq.AppImage ./install/lib/plotjuggler/PlotJuggler.desktop -appimage -bundle-non-qt-libs -no-strip -extra-plugins=iconengines,imageformats

If you have problems with the dependencies of the PJ plugins, additionally:

    cp src/PlotJuggler/PlotJuggler.desktop ./AppDir/
    cp src/PlotJuggler/plotjuggler.png     ./AppDir/
    cp src/PlotJuggler/AppRun              ./AppDir/
    
    ~/appimagetool-x86_64.AppImage AppDir/

    

