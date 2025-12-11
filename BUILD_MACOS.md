# PlotJuggler - macOS Native Application Bundle

PlotJuggler has been configured to build as a native macOS application bundle (`.app`).

## Changes Made

### 1. Created macOS Bundle Resources

- **Icon File**: [`plotjuggler_app/plotjuggler.icns`](plotjuggler_app/plotjuggler.icns)
  - Multi-resolution icon (16x16 to 1024x1024) converted from plotjuggler.png

- **Info.plist Template**: [`plotjuggler_app/Info.plist.in`](plotjuggler_app/Info.plist.in)
  - Bundle identifier: `io.plotjuggler.application`
  - Minimum macOS version: 10.15 (Catalina)
  - High resolution support enabled
  - Application category: Developer Tools

### 2. Modified CMake Configuration

#### Root CMakeLists.txt

- Added macOS-specific build configuration section
- Set `GUI_TYPE` to `MACOSX_BUNDLE` on macOS
- Configured bundle metadata variables (identifier, version, icon)
- Set `PJ_PLUGIN_INSTALL_DIRECTORY` to `plotjuggler.app/Contents/MacOS` for bundle builds

#### plotjuggler_app/CMakeLists.txt

- Added `${GUI_TYPE}` to `add_executable()` to enable bundle creation
- Configured bundle properties (Info.plist path, icon file, metadata)
- Added icon file to bundle resources
- Updated install rules to install as BUNDLE on macOS

## Building

### Prerequisites

```bash
brew install cmake qt@5 protobuf mosquitto zeromq zstd git-lfs
```

### Quick Build

Use the provided build script:

```bash
./build_macos_app.sh
```

This will:

1. Configure the project with CMake
2. Build PlotJuggler and all plugins
3. Create the application bundle
4. Run `macdeployqt` to include Qt frameworks
5. Install to `build/install/plotjuggler.app`

### Manual Build

```bash
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$PWD/install ..
make -j$(sysctl -n hw.ncpu)
make install

# Deploy Qt frameworks
$(brew --prefix qt@5)/bin/macdeployqt install/plotjuggler.app
```

## Running

```bash
open build/install/plotjuggler.app
```

Or double-click `plotjuggler.app` in Finder.

## Application Structure

```
plotjuggler.app/
├── Contents/
│   ├── Info.plist           # Bundle metadata
│   ├── MacOS/
│   │   ├── plotjuggler      # Main executable
│   │   └── lib*.dylib       # Plugin libraries (17 plugins)
│   ├── Resources/
│   │   └── plotjuggler.icns # Application icon
│   └── Frameworks/          # Qt frameworks (added by macdeployqt)
│       ├── QtCore.framework
│       ├── QtWidgets.framework
│       ├── QtGui.framework
│       └── ... (other Qt dependencies)
```

## Creating a DMG for Distribution

To create a distributable DMG installer:

```bash
brew install create-dmg

create-dmg \
  --volname 'PlotJuggler' \
  --window-pos 200 120 \
  --window-size 800 400 \
  --icon-size 100 \
  --icon 'PlotJuggler.app' 200 190 \
  --hide-extension 'PlotJuggler.app' \
  --app-drop-link 600 185 \
  'PlotJuggler.dmg' \
  build/install/plotjuggler.app
```

## Plugin Support

All 17 plugins are automatically included in the bundle:

- Data Loaders: CSV, MCAP, ULog
- Data Streamers: Sample, UDP, WebSocket, MQTT, ZMQ
- Parsers: ROS1, ROS2, Protobuf, DataTamer, LineInflux
- Toolboxes: FFT, Lua Editor, Quaternion
- Publishers: CSV

Plugins are loaded from `plotjuggler.app/Contents/MacOS/` automatically.

## Bundle Size

The complete application bundle (with Qt frameworks) is approximately **87 MB**.

## Platform Support

- **Minimum macOS Version**: 10.15 (Catalina)
- **Architectures**: arm64 (Apple Silicon), x86_64 (Intel)
- **Qt Version**: Qt 5.x

## Code Signing and Notarization

**Important**: macOS 10.15+ requires applications to be code-signed. The build script automatically applies an ad-hoc signature for local use.

### Ad-hoc Signing (Local Use)

The build script automatically signs with:

```bash
codesign --deep --force --sign - plotjuggler.app
```

This allows the app to run on your Mac but not on other Macs.

### Manual Signing

If you need to sign manually:

```bash
cd build/install
codesign --deep --force --sign - plotjuggler.app
codesign -vvv plotjuggler.app  # Verify signature
```

### Distribution Signing

For distribution outside the App Store, you should:

1. **Code Sign** the application:

   ```bash
   codesign --deep --force --verify --verbose --sign "Developer ID Application: Your Name" \
     build/install/plotjuggler.app
   ```

2. **Notarize** with Apple:

   ```bash
   xcrun notarytool submit PlotJuggler.dmg \
     --apple-id "your@email.com" \
     --team-id "TEAMID" \
     --password "app-specific-password"
   ```

3. **Staple** the notarization ticket:

   ```bash
   xcrun stapler staple PlotJuggler.dmg
   ```

## Backwards Compatibility

The CMake configuration maintains compatibility with:

- Non-macOS platforms (Linux, Windows)
- ROS/ROS2 builds (catkin, ament)
- Console application builds (when GUI_TYPE is not set)

## Notes

- The build creates a relocatable bundle with all dependencies included
- Qt frameworks are bundled using `macdeployqt`
- System libraries (like liblua, libmosquitto, libzmq) may still require installation via Homebrew for runtime
- The bundle is ready to run on any macOS system with the same or newer OS version
- **Code signing is applied automatically** by the build script using ad-hoc signature

## Troubleshooting

### App crashes with "Code Signature Invalid"

If the app crashes immediately with a code signing error:

```
Exception Type: EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
Termination Reason: Namespace CODESIGNING, Code 2, Invalid Page
```

**Solution**: Sign the application:

```bash
cd build/install
codesign --deep --force --sign - plotjuggler.app
```

The build script now does this automatically, but if you modify the bundle manually, you'll need to re-sign.

### App doesn't appear in Dock or crashes on launch

- Check Console.app for crash logs under "User Reports"
- Verify bundle structure: `ls -la plotjuggler.app/Contents/`
- Check code signature: `codesign -vvv plotjuggler.app`
- Try running from terminal to see errors: `./plotjuggler.app/Contents/MacOS/plotjuggler`

### Plugins not loading

Plugins should be in `plotjuggler.app/Contents/MacOS/`. Verify:

```bash
ls plotjuggler.app/Contents/MacOS/*.dylib
```

All 17 plugin `.dylib` files should be present.
