Simple-WebSocket-Server
=================

A very simple, fast, multithreaded, platform independent WebSocket (WS) and WebSocket Secure (WSS) server and client library implemented using C++11, Asio (both Boost.Asio and standalone Asio can be used) and OpenSSL. Created to be an easy way to make WebSocket endpoints in C++.

See https://gitlab.com/eidheim/Simple-Web-Server for an easy way to make REST resources available from C++ applications. Also, feel free to check out the new C++ IDE supporting C++11/14/17: https://gitlab.com/cppit/jucipp. 

### Features

* RFC 6455 mostly supported: text/binary frames, fragmented messages, ping-pong, connection close with status and reason.
* Asynchronous message handling
* Thread pool if needed
* Platform independent
* WebSocket Secure support
* Timeouts, if any of SocketServer::timeout_request and SocketServer::timeout_idle are >0 (default: SocketServer::timeout_request=5 seconds, and SocketServer::timeout_idle=0 seconds; no timeout on idle connections)
* Simple way to add WebSocket endpoints using regex for path, and anonymous functions
* An easy to use WebSocket and WebSocket Secure client library
* C++ bindings to the following OpenSSL methods: Base64, MD5, SHA1, SHA256 and SHA512 (found in crypto.hpp)

### Usage

See [ws_examples.cpp](ws_examples.cpp) or [wss_examples.cpp](wss_examples.cpp) for example usage. 

### Dependencies

* Boost.Asio or standalone Asio
* OpenSSL libraries

### Compile

Compile with a C++11 supported compiler:

```sh
mkdir build
cd build
cmake ..
make
cd ..
```

#### Run server and client examples

### WS

```sh
./build/ws_examples
```

### WSS

Before running the WSS-examples, an RSA private key (server.key) and an SSL certificate (server.crt) must be created.

Then:
```
./build/wss_examples
```
