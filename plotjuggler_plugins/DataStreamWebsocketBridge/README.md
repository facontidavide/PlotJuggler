# PlotJuggler WebSocket Client Plugin

WebSocket Client plugin to integrate remote data into **PlotJuggler** through a WebSocket bridge.

This plugin allows PlotJuggler to connect to a WebSocket server (for example, a custom ROS2 bridge) without requiring direct DDS or ROS2 access.

---

## Features

- Configurable WebSocket server connection (IP + port)
- Dynamic topic discovery
- Real-time subscribe and unsubscribe
- Remote close and reconnection handling
- Native integration within the PlotJuggler plugin system

---

## Architecture

```
PlotJuggler  <->  WebSocket Client Plugin  <->  WebSocket Server (pj_ros_bridge or similar)
```

The server can run on:

- The same machine  
- Another laptop on the local network  
- A remote robot  

---

# Installation

## 1. Clone PlotJuggler

```bash
mkdir -p ~/ws_plotjuggler/src
cd ~/ws_plotjuggler/src
git clone https://github.com/PlotJuggler/PlotJuggler.git
```

---

## 2. Clone the plugin inside `plotjuggler_plugins`

```bash
cd PlotJuggler/plotjuggler_plugins
git clone https://github.com/Alvvalencia/DataStreamWebsocketBridge.git
```

The final structure should look like:

```
PlotJuggler/
 ├── plotjuggler_plugins/
 │    ├── DataStreamWebsocketBridge/
 │    ├── ...
```

---

## 3. Add the plugin to `plotjuggler_plugins/CMakeLists.txt`

Open:

```
PlotJuggler/plotjuggler_plugins/CMakeLists.txt
```

Add your plugin directory:

```cmake
add_subdirectory(DataStreamWebsocketBridge)
```

Place it alongside the other plugin `add_subdirectory(...)` entries.

---

## 4. Install dependencies

### Ubuntu with Qt6

```bash
sudo apt install \
    qt6-base-dev \
    qt6-websockets-dev \
    libzstd-dev
```

### Ubuntu with Qt5

```bash
sudo apt install \
    qtbase5-dev \
    qtwebsockets5-dev \
    libzstd-dev
```

---

## 5. Build

```bash
cd ~/ws_plotjuggler
mkdir build
cd build
cmake ../src/PlotJuggler -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

# Usage

1. Run PlotJuggler:

```bash
./bin/plotjuggler
```

2. Navigate to:

```
Streaming → WebSocket Client
```

3. Configure:
   - Server IP (e.g., 192.168.1.50)
   - Port (e.g., 8080)

4. Connect.

5. Select topics and subscribe.

---

# Remote Connection

If the server listens on:

```
ws://0.0.0.0:8080
```

It accepts connections from any network interface.

From another machine, connect using:

```
ws://SERVER_IP:8080
```

Example:

```
ws://192.168.1.42:8080
```

---

# Typical ROS2 Setup

If using your ROS2 bridge:

```bash
ros2 run pj_ros_bridge pj_ros_bridge_node
```

Default configuration:

- Port: 8080  
- Publish rate: 50 Hz  

Then connect the plugin to:

```
ws://localhost:8080
```

or to the robot’s IP address.

---

# Connection States

The plugin handles the following states:

- Connecting  
- Connected  
- Disconnected  
- Remote Close  
- Error  

If the server closes the connection, the plugin detects the event and displays a warning in the UI.

---

# Development

Typical plugin structure:

```
DataStreamWebsocketBridge/
 ├── websocket_client.h
 ├── websocket_client.cpp
 ├── websocket_client.ui
 ├── CMakeLists.txt
 └── resources/
```

It integrates using PlotJuggler’s DataStream interface.

---

# License