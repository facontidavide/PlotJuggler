# PlotJuggler WebSocket Client Plugin

Plugin de **WebSocket Client** para integrar datos remotos en **PlotJuggler** mediante un bridge WebSocket.

Este plugin permite conectar PlotJuggler a un servidor WebSocket (por ejemplo un bridge ROS2 personalizado) sin necesidad de que PlotJuggler tenga acceso directo a DDS o ROS2.

---

## 🚀 Características

- Conexión a servidor WebSocket configurable (IP + puerto)
- Descubrimiento dinámico de topics
- Suscripción y desuscripción en tiempo real
- Soporte de compresión (ZSTD)
- Manejo de reconexión y cierre remoto
- Integración nativa dentro del sistema de plugins de PlotJuggler

---

## 🏗 Arquitectura

```
PlotJuggler  ←→  WebSocket Client Plugin  ←→  WebSocket Server (pj_ros_bridge u otro)
```

El servidor puede estar en:
- La misma máquina
- Otro portátil en red local
- Un robot remoto

---

# 🔧 Instalación

## 1️⃣ Clonar PlotJuggler

```bash
mkdir -p ~/ws_plotjuggler/src
cd ~/ws_plotjuggler/src
git clone https://github.com/PlotJuggler/PlotJuggler.git
```

---

## 2️⃣ Clonar el plugin dentro de plotjuggler_plugins

```bash
cd PlotJuggler/plotjuggler_plugins
git clone <URL_DE_TU_REPO> DataStreamWebsocketBridge
```

La estructura final debe quedar así:

```
PlotJuggler/
 ├── plotjuggler_plugins/
 │    ├── DataStreamWebsocketBridge/
 │    ├── ...
```

---

## 3️⃣ Instalar dependencias

### Ubuntu con Qt6

```bash
sudo apt install \
    qt6-base-dev \
    qt6-websockets-dev \
    libzstd-dev
```

### Ubuntu con Qt5

```bash
sudo apt install \
    qtbase5-dev \
    qtwebsockets5-dev \
    libzstd-dev
```

---

## 4️⃣ Compilar

```bash
cd ~/ws_plotjuggler
mkdir build
cd build
cmake ../src/PlotJuggler -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

# ▶️ Uso

1. Ejecutar PlotJuggler:

```bash
./bin/plotjuggler
```

2. Ir a:

```
Streaming → WebSocket Client
```

3. Configurar:
   - IP del servidor (ej: 192.168.1.50)
   - Puerto (ej: 8080)
   - Protocolo (ZSTD si está habilitado)

4. Conectar.

5. Seleccionar topics y suscribirse.

---

# 🌐 Conexión remota

Si el servidor escucha en:

```
ws://0.0.0.0:8080
```

Significa que acepta conexiones desde cualquier IP.

Desde otro portátil debes usar:

```
ws://IP_DEL_SERVIDOR:8080
```

Ejemplo:

```
ws://192.168.1.42:8080
```

---

# ⚙️ Configuración típica con ROS2

Si usas tu bridge ROS2:

```bash
ros2 run pj_ros_bridge pj_ros_bridge_node
```

Por defecto:
- Puerto: 8080
- Frecuencia: 50 Hz

Luego conecta el plugin a:

```
ws://localhost:8080
```

o a la IP del robot.

---

# 🧠 Estados de conexión

El plugin gestiona:

- Connecting
- Connected
- Disconnected
- Remote Close
- Error

Si el servidor cierra la conexión, el plugin detecta el evento y muestra advertencia en la interfaz.

---

# 🛠 Desarrollo

Estructura típica del plugin:

```
DataStreamWebsocketBridge/
 ├── websocket_client.h
 ├── websocket_client.cpp
 ├── CMakeLists.txt
 └── resources/
```

Se integra usando la interfaz de DataStream de PlotJuggler.

---

# 📜 Licencia

MIT / Apache 2.0 (según definas en tu repositorio)
