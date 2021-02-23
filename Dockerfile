FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  wget \
  cmake \
  qtbase5-dev \
  libqt5svg5-dev \
  libqt5websockets5-dev \
  libqt5opengl5-dev \
  libqt5x11extras5-dev \
  libbfd-dev \
  libdwarf-dev \
  libdw-dev \
  libbz2-dev \
  libboost-all-dev \
  autoconf \
  automake \
  libqwt-dev

RUN set -e && \
  cd /tmp && \
  VERSION=0.7.0 && \
  wget --no-check-certificate https://capnproto.org/capnproto-c++-${VERSION}.tar.gz && \
  tar xvf capnproto-c++-${VERSION}.tar.gz && \
  cd capnproto-c++-${VERSION} && \
  CXXFLAGS="-fPIC" ./configure && \
  make -j$(nproc) && \
  make install

RUN cd /tmp && \
  wget --no-check-certificate https://www.sourceware.org/pub/bzip2/bzip2-latest.tar.gz && \
  tar xvfz bzip2-latest.tar.gz && \
  cd bzip2-1.0.8 && \
  CFLAGS="-fPIC" make -f Makefile-libbz2_so && \
  make && \
  make install && \
  rm /usr/lib/x86_64-linux-gnu/libbz2.a && \
  rm /usr/lib/x86_64-linux-gnu/libbz2.so
