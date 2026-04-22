# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PlotJuggler is a Qt5-based C++17 desktop application for visualizing time series data. It supports file loading, real-time streaming, data transformation, and export via a dynamic plugin system. Licensed under MPL-2.0.

## Build Commands

```bash
# Linux (Ubuntu) - install deps first:
# sudo apt -y install qtbase5-dev libqt5svg5-dev libqt5websockets5-dev libqt5serialport5-dev \
#   libqt5opengl5-dev libqt5x11extras5-dev libprotoc-dev libzmq3-dev liblz4-dev libzstd-dev

# Configure and build
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=install
cmake --build build --config RelWithDebInfo --target install

# With Conan (Windows/cross-platform)
conan install . --output-folder build --build missing
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake \
      -DCMAKE_INSTALL_PREFIX=install -DBUILDING_WITH_CONAN=ON
cmake --build build --config RelWithDebInfo --target install

# Run
./build/bin/plotjuggler
```

Key CMake options: `ENABLE_ASAN`, `BASE_AS_SHARED` (ON for ROS2), `BUILDING_WITH_CONAN`, `PJ_PLUGINS_DIRECTORY`.

## Testing

```bash
# Build with tests enabled
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
cd build && ctest

# CSV parser tests (main test target)
./build/plotjuggler_plugins/DataLoadCSV/test_csv_parser
```

Tests use Google Test. Currently limited to `DataLoadCSV/tests/test_csv_parser.cpp`. Most validation is done via CI build verification across platforms.

## Code Style

- Google C++ Style with customizations (see `.clang-format`)
- 2-space indent, 100 char line limit, braces on new lines for classes/functions/namespaces
- Classes: PascalCase, methods: camelCase, member vars: underscore suffix
- All source files need MPL-2.0 license header
- Pre-commit hooks: `pre-commit install && pre-commit run -a`
- Excluded from formatting: `3rdparty/`, `contrib/`, `cmake/CPM.cmake`

## Architecture

Three main layers:

- **`plotjuggler_base/`** - Core library. Data structures (`PlotDataBase`, `TimeseriesBase`, `PlotDataMapRef`), plugin interfaces (`DataLoader`, `DataStreamer`, `StatePublisher`, `TransformFunction`, `ToolboxPlugin`, `ParserFactoryPlugin`), and widget base classes. Everything lives in the `PJ` namespace.

- **`plotjuggler_app/`** - Qt GUI application. `MainWindow` orchestrates everything: owns the central `PlotDataMapRef`, loads plugins via `PluginManager`, manages layout with `TabbedPlotWidget` > `PlotDocker` > `PlotWidget` hierarchy. Plot rendering uses Qwt. State persistence via XML (QDomDocument).

- **`plotjuggler_plugins/`** - Dynamically loaded plugins. Each plugin is a directory with its own `CMakeLists.txt`, uses Qt plugin system (`Q_OBJECT`, `Q_PLUGIN_METADATA`, `Q_INTERFACES`).

## Plugin Development

Plugin types and their IIDs:
- `DataLoader` - `"facontidavide.PlotJuggler3.DataLoader"`
- `DataStreamer` - `"facontidavide.PlotJuggler3.DataStreamer"`
- `StatePublisher` - `"facontidavide.PlotJuggler3.StatePublisher"`
- `TransformFunction` - `"facontidavide.PlotJuggler3.TransformFunction"`
- `TransformFunction_SISO` - `"facontidavide.PlotJuggler3.TransformFunctionSISO"`
- `ToolboxPlugin` - `"facontidavide.PlotJuggler3.Toolbox"`
- `ParserFactoryPlugin` - `"facontidavide.PlotJuggler3.ParserFactoryPlugin"`

Use `DataStreamSample` as the simplest reference plugin. `DataLoadCSV` shows how to separate parsing logic from the Qt plugin layer.

## Key Conventions

- Qt5 only (not Qt6). `CMAKE_AUTOMOC` and `CMAKE_AUTOUIC` are enabled.
- `DataStreamer` plugins must protect `dataMap()` access with `mutex()` for thread safety.
- Transforms execute in the main thread. `TransformFunction_SISO` is the common single-input/single-output variant.
- Lua scripting via sol2 for custom reactive transforms (`ReactiveLuaFunction`).
- ROS support is optional: can build with catkin (ROS1) or ament_cmake (ROS2), or standalone.
- Sample data files for manual testing in `datasamples/`.
