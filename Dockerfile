FROM ubuntu:22.04 as builder

RUN apt-get update && \
    apt-get -y install git cmake build-essential wget file qtbase5-dev libqt5svg5-dev \
                       libqt5websockets5-dev libqt5opengl5-dev libqt5x11extras5-dev \
                       libprotoc-dev libzmq3-dev liblz4-dev libzstd-dev libmosquittopp-dev

RUN mkdir -p /opt/plotjuggler
COPY . /opt/plotjuggler
RUN mkdir /opt/plotjuggler/build
WORKDIR /opt/plotjuggler/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
RUN make -j `nproc`
RUN make install DESTDIR=AppDir
RUN /opt/plotjuggler/appimage/AppImage.sh

FROM scratch AS exporter
COPY --from=builder /opt/plotjuggler/build/PlotJuggler-x86_64.AppImage /
