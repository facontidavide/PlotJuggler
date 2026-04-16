# Serial Port DataStreamer Plugin

Streams data from a serial port into PlotJuggler. Useful for microcontrollers, embedded devices, and any hardware that sends structured data over UART/USB-serial.

## Dependencies

This plugin requires `Qt5SerialPort`. It is **optional** — if the library is not found at build time, the plugin is skipped and the rest of PlotJuggler builds normally.

### Ubuntu / Debian

```bash
sudo apt install libqt5serialport5-dev
```

### Fedora

```bash
sudo dnf install qt5-qtserialport-devel
```

## Supported Protocols

Currently only **JSON** is supported. The device must send newline-separated or back-to-back JSON objects, e.g.:

```
{"temperature":23.5,"pressure":1013.2}
{"temperature":23.6,"pressure":1013.1}
```

Nested objects are handled correctly:

```
{"sensor":{"temp":23.5,"hum":60},"ts":1234}
```

Arbitrary bytes between JSON objects (e.g. debug prints, blank lines) are ignored — the splitter looks for matching `{` ... `}` pairs.

## Usage

1. Open PlotJuggler
2. Go to **Streaming** and select **Serial Port**
3. Configure:
   - **Port** — select from detected serial devices (click Refresh to rescan)
   - **Baud rate** — must match your device (common: 115200, 9600)
   - **Data bits / Parity / Stop bits / Flow control** — match your device settings
   - **Protocol** — select JSON and configure the message parser
4. Click OK to start streaming

Settings are remembered between sessions.

## Notes

- **Parse errors are non-blocking.** If a message fails to parse, the plugin increments a failure counter shown in the notification bar rather than stopping the stream. Click the notification to see the count and reset it.
- **Port selection is robust.** The plugin identifies ports by system path, not by the display string, so devices with unusual manufacturer names won't cause issues.
- **Buffer safety.** The JSON splitter caps its internal buffer at 1 MB to prevent runaway memory usage from malformed data. If exceeded, the buffer is reset and the splitter resynchronizes on the next `{`.
