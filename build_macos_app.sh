#!/bin/bash
# Build script for PlotJuggler native macOS application bundle

set -e

BUILD_DIR="build"
INSTALL_DIR="$BUILD_DIR/install"

echo "==> Building PlotJuggler as native macOS application..."

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "==> Configuring with CMake..."
cmake -DCMAKE_INSTALL_PREFIX="$PWD/install" ..

# Build
echo "==> Building..."
make -j$(sysctl -n hw.ncpu)

# Install
echo "==> Installing to $INSTALL_DIR..."
make install

# Deploy Qt frameworks
echo "==> Deploying Qt frameworks with macdeployqt..."
QT5_PATH=$(brew --prefix qt@5)
if [ -f "$QT5_PATH/bin/macdeployqt" ]; then
    "$QT5_PATH/bin/macdeployqt" install/plotjuggler.app -verbose=1
else
    echo "Warning: macdeployqt not found. Install Qt5 via Homebrew: brew install qt@5"
fi

# Code sign the application (ad-hoc signature for local use)
echo "==> Code signing application..."
codesign --deep --force --sign - install/plotjuggler.app
if [ $? -eq 0 ]; then
    echo "==> Code signing successful (ad-hoc signature)"
else
    echo "Warning: Code signing failed. The app may not run on macOS 10.15+"
fi

echo ""
echo "==> Build complete!"
echo "==> PlotJuggler.app is located at: $INSTALL_DIR/plotjuggler.app"
echo "==> To run: open $INSTALL_DIR/plotjuggler.app"
echo ""
echo "Bundle size:"
du -sh install/plotjuggler.app
echo ""
echo "To create a DMG for distribution, you can use:"
echo "  brew install create-dmg"
echo "  create-dmg --volname 'PlotJuggler' --window-pos 200 120 --window-size 800 400 \\"
echo "    --icon-size 100 --icon 'PlotJuggler.app' 200 190 --hide-extension 'PlotJuggler.app' \\"
echo "    --app-drop-link 600 185 'PlotJuggler.dmg' install/plotjuggler.app"
