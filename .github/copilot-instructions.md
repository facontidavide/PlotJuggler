# PlotJuggler - GitHub Copilot Instructions

## Project Overview

PlotJuggler is a Qt-based desktop application for visualizing time series data. It's a cross-platform tool (Linux, macOS, Windows) that supports various data formats and provides real-time streaming capabilities.

### Key Technologies
- **Language**: C++ (C++17 standard)
- **GUI Framework**: Qt5 (Widgets, OpenGL, WebSockets, SVG, PrintSupport, Concurrent, Xml, Network)
- **Build System**: CMake (minimum version 3.16)
- **Scripting**: Lua (for custom transforms)
- **Additional Libraries**: fmt, nlohmann-json, lz4, zstd, protobuf, ZeroMQ
- **ROS Support**: Compatible with both ROS1 (catkin) and ROS2 (ament_cmake)

### Architecture
- **plotjuggler_base**: Core library providing base classes for plugins and data handling
- **plotjuggler_app**: Main application with GUI components
- **plotjuggler_plugins**: Plugin system supporting:
  - Data Loaders (CSV, ULog, MCAP, Parquet)
  - Data Streamers (MQTT, WebSocket, UDP, ZMQ, ROS)
  - State Publishers (CSV, ZMQ)
  - Toolboxes (FFT, Lua Editor, Quaternion)
  - Parsers (Protobuf, ROS messages, InfluxDB, Data Tamer, IDL)

## Build Instructions

### Linux (Ubuntu 20.04/22.04)

```bash
# Install dependencies
sudo apt -y install qtbase5-dev libqt5svg5-dev libqt5websockets5-dev \
      libqt5opengl5-dev libqt5x11extras5-dev libprotoc-dev libzmq3-dev \
      liblz4-dev libzstd-dev

# Clone repository
git clone https://github.com/facontidavide/PlotJuggler.git ~/plotjuggler_ws/src/PlotJuggler
cd ~/plotjuggler_ws

# Build with CMake
cmake -S src/PlotJuggler -B build/PlotJuggler -DCMAKE_INSTALL_PREFIX=install
cmake --build build/PlotJuggler --config RelWithDebInfo --target install
```

### macOS

```bash
# Install dependencies
brew install cmake qt@5 protobuf mosquitto zeromq zstd git-lfs

# Set environment variables
QT_HOME=$(brew --prefix qt@5)
export CPPFLAGS="-I $QT_HOME/include"
export PKG_CONFIG_PATH="$QT_HOME/lib/pkgconfig"
export LDFLAGS="-L$QT_HOME/lib"

# Build
cmake -S src/PlotJuggler -B build/PlotJuggler -DCMAKE_INSTALL_PREFIX=install -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build/PlotJuggler --config RelWithDebInfo --target install
```

### Windows

Uses Conan or vcpkg for dependency management. Visual Studio 2019 (16) required.

```batch
# With Conan
conan install src/PlotJuggler --install-folder build/PlotJuggler --build=missing -pr:b=default
set CMAKE_TOOLCHAIN=%cd%/build/PlotJuggler/conan_toolchain.cmake
cmake -G "Visual Studio 16" -S src/PlotJuggler -B build/PlotJuggler -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN% -DCMAKE_INSTALL_PREFIX=%cd%/install -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
cmake --build build/PlotJuggler --config Release --target install
```

### ROS Build

For ROS1:
```bash
# Build with catkin
catkin build plotjuggler
```

For ROS2:
```bash
# Build with colcon
colcon build --packages-select plotjuggler
```

## Coding Standards

### Code Style
- **Style Guide**: Based on Google C++ Style Guide with customizations
- **Formatter**: clang-format (configuration in `.clang-format`)
- **Line Length**: 100 characters maximum
- **Indentation**: 2 spaces (no tabs)
- **Braces**: Custom style with braces on new lines for classes, functions, structs, enums, namespaces
- **Naming Conventions**:
  - Classes: PascalCase (e.g., `PlotWidget`, `DataLoader`)
  - Functions/Methods: camelCase (e.g., `getData()`, `transformData()`)
  - Member variables: Use underscore suffix convention where applicable
  - Pointers: `Type* pointer` (pointer binds to type)

### Pre-commit Hooks
The project uses pre-commit hooks for code quality:
```bash
# Install pre-commit hooks
pre-commit install

# Run manually
pre-commit run -a
```

Hooks include:
- clang-format for C++ code formatting
- Standard checks (large files, merge conflicts, trailing whitespace, etc.)
- XML and YAML validation

### File Headers
All source files should include the MPL-2.0 license header:
```cpp
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
```

## Plugin Development

### Plugin Types
1. **DataLoader**: Load data from files (inherit from `DataLoader` base class)
2. **DataStreamer**: Stream data in real-time (inherit from `DataStreamer` base class)
3. **StatePublisher**: Publish/export data (inherit from `StatePublisher` base class)
4. **TransformFunction**: Transform time series data (inherit from `TransformFunction` base class)
5. **ToolboxPlugin**: Add custom tools/widgets (inherit from `ToolboxPlugin` base class)

### Plugin Structure
- Each plugin is a separate directory in `plotjuggler_plugins/`
- Must have its own `CMakeLists.txt`
- Plugins are loaded dynamically at runtime
- Use Qt's plugin system (Q_OBJECT, Q_PLUGIN_METADATA)

### Example Plugin Locations
- Sample plugins: https://github.com/PlotJuggler/plotjuggler-sample-plugins
- ROS plugins: https://github.com/PlotJuggler/plotjuggler-ros-plugins
- MQTT plugin: https://github.com/PlotJuggler/plotjuggler-mqtt
- LSL plugin: https://github.com/PlotJuggler/plotjuggler-lsl

## Testing

### Current State
- The project has limited automated testing infrastructure
- Main validation is done through CI/CD workflows for different platforms
- Testing primarily focuses on build verification across platforms

### CI/CD Workflows
Located in `.github/workflows/`:
- `ubuntu.yaml`: Ubuntu build and testing
- `windows.yaml`: Windows build verification
- `macos.yaml`: macOS build verification
- `ros2-*.yaml`: ROS2 integration testing (humble, jazzy, rolling)
- `debian.yaml`: Debian package build
- `snap_core*.yaml`: Snap package builds
- `codeql.yml`: Security analysis
- `pre-commit.yaml`: Code formatting and quality checks validation

### Manual Testing
- Build the project on target platform
- Run the application: `./build/PlotJuggler/bin/plotjuggler`
- Load sample data from `datasamples/` directory
- Test plugin functionality by loading specific data formats

## Important Notes

### Dependencies Management
- Use system packages when possible (apt, brew, etc.)
- Conan or vcpkg for Windows builds
- CMake's FetchContent for some third-party libraries
- Git submodules avoided (empty .gitmodules file)

### Qt Specifics
- Uses Qt5 (not Qt6)
- MOC (Meta-Object Compiler) enabled via `CMAKE_AUTOMOC`
- UIC (User Interface Compiler) enabled via `CMAKE_AUTOUIC`
- Resource files compiled automatically

### Cross-Platform Considerations
- Windows-specific: `WIN32` flag, special handling for executables
- macOS-specific: Homebrew Qt paths, RPATH settings
- Linux-specific: Address Sanitizer support, backward-cpp for stack traces

### ROS Integration
- Supports both ROS1 (Noetic) and ROS2 (Humble, Jazzy, Rolling)
- Conditional compilation based on ROS version
- ROS-specific plugins in separate repository
- Can be built without ROS support

### Version Information
- Version info embedded in compiled binary
- Installation type tracked via `PJ_INSTALLATION` CMake variable

## Common Tasks

### Adding a New Plugin
1. Create directory in `plotjuggler_plugins/`
2. Implement plugin class inheriting from appropriate base class
3. Add CMakeLists.txt with `add_plotjuggler_plugin()` macro
4. Register plugin with Qt plugin system
5. Add to parent CMakeLists.txt if needed

### Modifying Core Functionality
- Core data structures in `plotjuggler_base/include/PlotJuggler/plotdata.h`
- Base classes for plugins in `plotjuggler_base/include/PlotJuggler/`
- Main window logic in `plotjuggler_app/mainwindow.cpp`
- Plot widget implementation in `plotjuggler_app/plotwidget.cpp`

### Adding Dependencies
1. Update CMakeLists.txt with find_package() or FetchContent
2. Update package.xml for ROS builds
3. Update COMPILE.md with platform-specific installation instructions
4. Consider Conan and vcpkg support for Windows

## Resources

- **Documentation**: `README.md`, `COMPILE.md`
- **License**: Mozilla Public License Version 2.0 (MPL-2.0)
- **Main Repository**: https://github.com/facontidavide/PlotJuggler
- **Issue Tracker**: GitHub Issues
- **Tutorials**: https://slides.com/davidefaconti/
- **Changelog**: `CHANGELOG.rst`

## Development Tips

1. **Use pre-commit hooks** to ensure code quality before committing
2. **Test on target platforms** - CI runs on Ubuntu, Windows, and macOS
3. **Follow the existing plugin pattern** when adding new functionality
4. **Keep plugins modular** - they should be independently loadable
5. **Use Qt signals/slots** for UI interactions and event handling
6. **Leverage the fmt library** for string formatting (already included)
7. **Check existing plugins** for examples before implementing new features
8. **Consider ROS compatibility** if adding data streaming features
9. **Update CHANGELOG.rst** for user-facing changes
10. **Keep performance in mind** - PlotJuggler handles millions of data points
