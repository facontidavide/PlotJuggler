#!/bin/bash

APPDIRPATH=../build/AppDir

if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x linuxdeploy-x86_64.AppImage
fi

if [ ! -f "linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
    chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
fi



# Populate AppDir with Qt libraries / plugins. No --output yet: we need
# to patch the auto-generated apprun-hooks/linuxdeploy-plugin-qt-hook.sh
# before packaging.
./linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
    --appdir "$APPDIRPATH" \
    -d ../io.plotjuggler.PlotJuggler.desktop \
    -i ../plotjuggler.png \
    --plugin qt \
    || { echo "ERROR: linuxdeploy Qt deploy stage failed" >&2; exit 1; }

# linuxdeploy-plugin-qt auto-injects `export QT_QPA_PLATFORMTHEME=gtk2`
# on GNOME/XFCE for "native look" — but we don't bundle a gtk2
# platformtheme plugin, so Qt silently falls back and fonts/icons
# break. Fusion is already forced from main.cpp; drop just that line.
sed -i '/QT_QPA_PLATFORMTHEME/d' \
    "$APPDIRPATH/apprun-hooks/linuxdeploy-plugin-qt-hook.sh" \
    || { echo "ERROR: failed to patch linuxdeploy-plugin-qt hook" >&2; exit 1; }

# Now package the patched AppDir into an AppImage.
./linuxdeploy-x86_64.AppImage --appimage-extract-and-run \
    --appdir "$APPDIRPATH" \
    --output appimage \
    || { echo "ERROR: AppImage packaging stage failed" >&2; exit 1; }
