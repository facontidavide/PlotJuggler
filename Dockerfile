FROM ubuntu:22.04 AS builder

RUN apt-get update && \
    apt-get -y install git cmake build-essential wget file ca-certificates lsb-release pkg-config \
                       qtbase5-dev libqt5svg5-dev \
                       libqt5websockets5-dev libqt5serialport5-dev libqt5opengl5-dev libqt5x11extras5-dev \
                       libprotoc-dev libzmq3-dev liblz4-dev libzstd-dev libmosquittopp-dev

# Apache Arrow + Flight are required by the ToolboxMosaico plugin. Without
# them the plugin silently returns early from its CMakeLists.txt and is
# never built or installed. Ubuntu 22.04's default archives don't ship
# Arrow, so pull from the official Apache Arrow APT repo.
# libgrpc++-dev + pkg-config are required by Arrow's FindgRPCAlt.cmake —
# Ubuntu's gRPC 1.30 doesn't ship CMake config files, so Arrow falls back
# to pkg-config, which is why pkg-config is in the apt install above.
RUN wget -q https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get install -y ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get update && \
    apt-get install -y libarrow-dev libarrow-flight-dev libgrpc++-dev && \
    rm apache-arrow-apt-source-latest-*.deb

RUN mkdir -p /opt/plotjuggler
COPY . /opt/plotjuggler
RUN mkdir /opt/plotjuggler/build
WORKDIR /opt/plotjuggler/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DPJ_PLUGINS_DIRECTORY="bin"
RUN make -j `nproc`
RUN make install DESTDIR=AppDir
ENV APPIMAGE_EXTRACT_AND_RUN=1
RUN /opt/plotjuggler/appimage/AppImage.sh

FROM scratch AS exporter
COPY --from=builder /opt/plotjuggler/build/PlotJuggler-x86_64.AppImage /
